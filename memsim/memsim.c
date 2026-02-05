#include "memsim.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int createMMU(mmu *mmu, int frames) {
  memset(mmu, 0, sizeof(*mmu));

  mmu->numFrames = frames;

  mmu->page_table = (int *)malloc(NUM_PAGES * sizeof(int));
  mmu->frame_data = (frame_entry *)calloc(frames, sizeof(frame_entry));

  if (!mmu->page_table || !mmu->frame_data) {
    free(mmu->frame_data);
    free(mmu->page_table);
    return -1;
  }

  for (int i = 0; i < (int)NUM_PAGES; i++) {
    mmu->page_table[i] = -1;
  }
  for (int f = 0; f < frames; f++) {
    mmu->frame_data[f].vpn = -1;
  }
  return 0;
}

void destroyMMU(mmu *mmu) {
  free(mmu->frame_data);
  mmu->frame_data = NULL;
  free(mmu->page_table);
  mmu->page_table = NULL;
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
evicted_page replacePage(mmu *mmu, int vpn, enum repl mode, int *new_frame) {
  evicted_page victim;
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
  victim.vpn = mmu->frame_data[victim_frame].vpn;
  victim.dirty = mmu->frame_data[victim_frame].dirty;

  // Flag victim as not being present in memory
  if (victim.vpn != -1) {
    mmu->page_table[victim.vpn] = -1;
  }

  // Map new page into victim frame
  mmu->page_table[vpn] = victim_frame;
  mmu->frame_data[victim_frame].vpn = vpn;

  // Reset metadata for the frame
  mmu->frame_data[victim_frame].access_time = mmu->time++;
  mmu->frame_data[victim_frame].dirty = 0;
  mmu->frame_data[victim_frame].ref = 1;

  if (new_frame) {
    *new_frame = victim_frame;
  }

  return victim;
}
