#include "memsim.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// lru_node and lru_tracker are needed for advanced LRU. The doubly linked
// list maintains frames in access order; LRU is the tail and MRU is the head.
// Eviction always occurs from the tail, making it O(1). A cache hit moves
// the node from it's current location in the DLL to head, relinking as
// appropriate

// A hashmap (VPN -> node) was considered but is not needed. The page table
// already provides the VPN -> node mapping in two O(1) steps:
// page_table[vpn] gives the PFN, and frame_data[pfn].node is the DLL node
// embedded directly in that frame's metadata. No scanning occurs for either
// step, both are direct array and struct field accesses.

typedef struct lru_node {
  struct lru_node *prev;
  struct lru_node *next;
  int frame; // what PFN is this virtual page in
} lru_node;

typedef struct {
  lru_node *head;
  lru_node *tail;
} lru_tracker;

// Per-frame meta-data, needed by the page replacement policies
typedef struct {
  int vpn;   // which page is in this frame, -1 if free
  int dirty; // if the frame has been written to, 0 or 1 for clean or dirty
  int access_time; // simple LRU timestamp
  int ref;         // clock ref bit
  lru_node node;   // each frame has a DLL node
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
  lru_tracker lru_state; // owns pointers to head and tail of the linked list
  enum repl
      mode; // active replacement policy. Stored here so that check_in_memory()
            // and allocate_frame() can update only the relevent metadata
            // without needing the mode passed as a parameter to every call.
};

// Inserts a new node at the head
// This function can only be used safely when the node is new to the list.
// Calling this on a node that is already in the list will corrupt the list.
static void lru_new_node_push_head(lru_tracker *lru, lru_node *node) {
  node->prev = NULL;
  node->next = lru->head;

  if (lru->head) {
    lru->head->prev = node;
  }
  lru->head = node;
  if (lru->tail == NULL) {
    lru->tail = node;
  }
}

// Move a node from within the list to the head. Assumes that the node is within
// the list
static void lru_move_existing_node_head(lru_tracker *lru, lru_node *node) {
  if (lru->head == node) {
    return;
  }

  // unlink from current position
  if (node->prev) {
    node->prev->next = node->next;
  }
  if (node->next) {
    node->next->prev = node->prev;
  }
  if (lru->tail == node) {
    lru->tail = node->prev;
  }

  // insert at head
  node->prev = NULL;
  node->next = lru->head;
  lru->head->prev = node;
  lru->head = node;
}

int has_free_frames(mmu *mmu_ptr) {
  if (mmu_ptr->next_frame < mmu_ptr->numFrames) {
    return 1;
  }
  return 0;
}

void mark_dirty(mmu *mmu_ptr, int pfn) { mmu_ptr->frame_data[pfn].dirty = 1; }

mmu *create_MMU(int frames, enum repl mode) {
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
  // init each frame's vpn to -1 to indicate no pages in memory. Init the
  // embedded lru node's frame field to its own PFN so that eviction can
  // obtain the PFN from the node in O(1) without a reverse lookup.
  for (int f = 0; f < frames; f++) {
    mmu_ptr->frame_data[f].vpn = -1;
    mmu_ptr->frame_data[f].node.frame = f;
  }

  mmu_ptr->lru_state.head = NULL;
  mmu_ptr->lru_state.tail = NULL;

  mmu_ptr->mode = mode;

  return mmu_ptr;
}

void destroy_MMU(mmu *mmu) {
  free(mmu->frame_data);
  mmu->frame_data = NULL;
  free(mmu->page_table);
  mmu->page_table = NULL;
  free(mmu);
}

// Checks if the page is in memory, returns frame number or -1 if not in memory
int check_in_memory(mmu *mmu, int vpn) {
  int pfn = mmu->page_table[vpn];

  if (pfn != -1) {
    if (mmu->mode == lru_simple) {
      mmu->frame_data[pfn].access_time = mmu->time++;
    } else if (mmu->mode == lru) {
      lru_move_existing_node_head(&mmu->lru_state, &mmu->frame_data[pfn].node);
    } else if (mmu->mode == _clock || mmu->mode == _clock_clean) {
      mmu->frame_data[pfn].ref = 1;
    }
    // fifo and random don't have metadata update on hit
  }
  return pfn;
}

// Place vpn into the next free physical frame. Frames are assigned vpns
// sequentially in the range [0,num_frames-1]. Only valid while free frames
// remain, intended use is to call has_free_frames() first to validate this
// assumption.
int allocate_frame(mmu *mmu, int vpn) {
  int pfn = mmu->next_frame++;
  mmu->page_table[vpn] = pfn;
  mmu->frame_data[pfn].vpn = vpn;
  mmu->frame_data[pfn].dirty = 0;
  mmu->frame_data[pfn].access_time = mmu->time++;
  mmu->frame_data[pfn].ref = 1;
  if (mmu->mode == lru) {
    lru_new_node_push_head(&mmu->lru_state, &mmu->frame_data[pfn].node);
  }
  return pfn;
}

// Selects a page for eviction according to the replacement algorithm
replace_result replace_page(mmu *mmu, int vpn) {
  replace_result page_info;
  int victim_frame = -1; // Initialise to -1 as a gaurd
  enum repl mode = mmu->mode;

  // LRU simple
  if (mode == lru_simple) {
    int oldest = mmu->frame_data[0].access_time;
    victim_frame = 0;
    // loop through all frames to find the one that was accessed least recently
    for (int i = 1; i < mmu->numFrames; i++) {
      if (mmu->frame_data[i].access_time < oldest) {
        oldest = mmu->frame_data[i].access_time;
        victim_frame = i;
      }
    }
    // LRU advanced
  } else if (mode == lru) {
    // tail is LRU
    lru_node *victim_node = mmu->lru_state.tail;
    victim_frame = victim_node->frame;

    // unlink from tail
    mmu->lru_state.tail = victim_node->prev;
    if (mmu->lru_state.tail) {
      mmu->lru_state.tail->next = NULL;
    } else {
      mmu->lru_state.head = NULL;
    }

    // The victim node is the tail of the DLL. Unlink it,
    // evict the page it represents, and load the incoming VPN into the same
    // physical frame. The node itself is reused and pushed to the head.
    lru_new_node_push_head(&mmu->lru_state, victim_node);
  }
  // random
  else if (mode == _random) {
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
