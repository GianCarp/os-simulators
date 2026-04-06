#include "memsim.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Per-frame meta-data, needed by the page replacement policies
typedef struct {
  int vpn;   // which page is in this frame, -1 if free
  int dirty; // if the frame has been written to, 0 or 1 for clean or dirty
  int access_time; // LRU timestamp
  int ref;         // clock ref bit
} frame_entry;

// In a real OS, the page table and physical frame metadata are separate
// structures owned by different parts of the kernel. The page table is
// per-process, stored in kernel space, and used for VPN to PFN address
// translation. The physical frame metadata (mem_map) is a global
// kernel structure used by the page replacement system to track the
// contents and state of each physical frame.
//
// Since this simulator models a single process, both are collapsed into
// this struct for simplicity.
struct mmu {
  int numFrames;
  int *page_table; // map VPN -> PFN, -1 if not resident
  frame_entry
      *frame_data; // contain meta data regarding access time, dirty bit, etc
  int time;        // counter for LRU timestamps
  int clock_hand;  // hand position for clock algorithm
  int fifo_hand;   // hand position for FIFO algorithm
  int next_frame;  // next free frame to allocate (sequential)
};

int has_free_frames(mmu *mmu_ptr) {
  if (mmu_ptr->next_frame < mmu_ptr->numFrames) {
    return 1;
  }
  return -1;
}

void mark_dirty(mmu *mmu_ptr, int pfn) { mmu_ptr->frame_data[pfn].dirty = 1; }

mmu *createMMU(int frames) {
  mmu *mmu_ptr = (mmu *)malloc(sizeof(mmu));
  if (mmu_ptr == NULL) {
    return NULL;
  }
  memset(mmu_ptr, 0, sizeof(*mmu_ptr));
  mmu_ptr->numFrames = frames;

  mmu_ptr->page_table = (int *)malloc(NUM_PAGES * sizeof(int));
  mmu_ptr->frame_data = (frame_entry *)calloc(frames, sizeof(frame_entry));

  if (!mmu_ptr->page_table || !mmu_ptr->frame_data) {
    free(mmu_ptr->frame_data);
    free(mmu_ptr->page_table);
    free(mmu_ptr);
    return NULL;
  }

  for (int i = 0; i < (int)NUM_PAGES; i++) {
    mmu_ptr->page_table[i] = -1;
  }
  for (int f = 0; f < frames; f++) {
    mmu_ptr->frame_data[f].vpn = -1;
  }
  return mmu_ptr;
}

void destroyMMU(mmu *mmu) {
  free(mmu->frame_data);
  mmu->frame_data = NULL;
  free(mmu->page_table);
  mmu->page_table = NULL;
  free(mmu);
}

// Checks if the page is in memory, returns frame no or -1 if not found
int checkInMemory(mmu *mmu, int vpn) {
  int result = mmu->page_table[vpn];

  if (result != -1) {
    mmu->frame_data[result].access_time = mmu->time++; // LRU
    mmu->frame_data[result].ref = 1;                   // clock
  }
  return result;
}

// Allocate the next free frame to 'vpn' (only valid while free frames remain).
int allocateFrame(mmu *mmu, int vpn) {
  int pfn = mmu->next_frame++;
  mmu->page_table[vpn] = pfn;

  mmu->frame_data[pfn].vpn = vpn;
  mmu->frame_data[pfn].dirty = 0;
  mmu->frame_data[pfn].access_time = mmu->time++;
  mmu->frame_data[pfn].ref = 1;

  return pfn;
}

// Selects a page for eviction according to the replacement algorithm
replace_result replacePage(mmu *mmu, int vpn, enum repl mode) {
  replace_result page_info;
  int victim_frame = -1; // Initialise to -1 as a gaurd

  // LRU
  if (mode == lru) {
    int oldest = mmu->frame_data[0].access_time;
    victim_frame = 0;
    // loop through all frames to find the one that was accessed least recently
    for (int i = 1; i < mmu->numFrames; i++) {
      if (mmu->frame_data[i].access_time < oldest) {
        oldest = mmu->frame_data[i].access_time;
        victim_frame = i;
      }
    }

    // random
  } else if (mode == _random) {
    // unbiased random selection in [0, numFrames)
    unsigned span = (unsigned)mmu->numFrames;
    unsigned limit = (RAND_MAX + 1u) - ((RAND_MAX + 1u) % span);
    unsigned r;

    do {
      r = rand();
    } while (r >= limit);
    victim_frame = r % span;

    // FIFO
  } else if (mode == fifo) {
    victim_frame = mmu->fifo_hand;
    mmu->fifo_hand = (mmu->fifo_hand + 1) % mmu->numFrames;

    // clock
  } else if (mode == _clock) {
    while (mmu->frame_data[mmu->clock_hand].ref == 1) {
      mmu->frame_data[mmu->clock_hand].ref = 0;
      mmu->clock_hand = (mmu->clock_hand + 1) % mmu->numFrames;
    }
    victim_frame = mmu->clock_hand;
    mmu->clock_hand = (mmu->clock_hand + 1) % mmu->numFrames;

    // clean clock
  } else if (mode == _clock_clean) {
    int scanned = 0;

    // Pass 1: prefer non-dirty pages
    while (scanned < mmu->numFrames) {
      if (mmu->frame_data[mmu->clock_hand].ref == 1) {
        mmu->frame_data[mmu->clock_hand].ref = 0;
      } else if (mmu->frame_data[mmu->clock_hand].ref == 0 &&
                 mmu->frame_data[mmu->clock_hand].dirty == 0) {
        victim_frame = mmu->clock_hand;
        mmu->clock_hand = (mmu->clock_hand + 1) % mmu->numFrames;
        break;
      }
      mmu->clock_hand = (mmu->clock_hand + 1) % mmu->numFrames;
      scanned++;
    }

    // Pass 2: if no clean victim found, evict first ref = 0
    if (victim_frame < 0) {
      while (mmu->frame_data[mmu->clock_hand].ref == 1) {
        mmu->frame_data[mmu->clock_hand].ref = 0;
        mmu->clock_hand = (mmu->clock_hand + 1) % mmu->numFrames;
      }
      victim_frame = mmu->clock_hand;
      mmu->clock_hand = (mmu->clock_hand + 1) % mmu->numFrames;
    }
  }
  // Defend against the case where victim_frame has not been changed from -1
  // or has been set to an invalid value
  if (victim_frame < 0) {
    fprintf(stderr, "[BUG] replacePage(): victim_frame not set (mode=%d)\n",
            mode);
    exit(EXIT_FAILURE);
  }

  // Victim page return value set-up
  page_info.victim.vpn = mmu->frame_data[victim_frame].vpn;
  page_info.victim.dirty = mmu->frame_data[victim_frame].dirty;

  // Flag victim as not being present in memory
  if (page_info.victim.vpn != -1) {
    mmu->page_table[page_info.victim.vpn] = -1;
  }

  // Map new page into victim frame
  mmu->page_table[vpn] = victim_frame;
  mmu->frame_data[victim_frame].vpn = vpn;

  // Reset metadata for the frame
  mmu->frame_data[victim_frame].access_time = mmu->time++;
  mmu->frame_data[victim_frame].dirty = 0;
  mmu->frame_data[victim_frame].ref = 1;

  page_info.new_pfn = victim_frame;

  return page_info;
}
