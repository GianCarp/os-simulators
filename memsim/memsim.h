#ifndef MEMSIM_H
#define MEMSIM_H

#include <stdint.h>

// 32-bit virtual address space, 4 Kb pages => 2^(32-12) = 2^20 pages
#define NUM_PAGES (1u << 20)

// Returned by replacePage(), contains information of the page that was evicted
typedef struct {
  int vpn;   // vpn of page evicted
  int dirty; // if the frame has been written to, 0 or 1 for clean or dirty
} evicted_page;

// Per-frame meta-data, needed by the page replacement policies
typedef struct {
  int vpn;   // which page is in this frame, -1 if free
  int dirty; // if the frame has been written to, 0 or 1 for clean or dirty
  int access_time; // LRU timestamp
  int ref;         // clock ref bit
} frame_entry;

// Internal simulator state. Model assumes a 4 KB page size and a 32-bit system,
// so up to 2^20 pages.
typedef struct {
  int numFrames;
  int *page_table; // map VPN -> PFN, -1 if not resident
  frame_entry
      *frame_data; // contain meta data regarding acces time, dirty bit, etc
  int time;        // counter for LRU timestamps
  int clock_hand;  // hand position for clock algorithm
  int fifo_hand;   // hand position for FIFO algorithm
  int next_frame;  // next free frame to allocate (sequential)
} mmu;

// Replacement policies supportd by the simulator
enum repl { _random, fifo, lru, _clock, _clock_clean };

// Core simulator API

// Initialise simulator state for a machine with 'frames' physical frames.
// Returns 0 on success, -1 on allocation failure.
int createMMU(mmu *mmu, int frames);

// Free memory allocated by createMMU().
void destroyMMU(mmu *mmu);

// Returns PFN if 'vpn' is in memory, or -1 if not (page fault).
// On a hit, updates metadata (LRU timestamp and CLOCK ref-bit).
int checkInMemory(mmu *mmu, int vpn);

// Allocate the next free frame to 'vpn' (only valid while free frames remain).
// Returns PFN assigned to this VPN, needed so that the frame can be marked as
// dirty if appropriate.
int allocateFrame(mmu *mmu, int vpn);

/* Perform page replacement for 'vpn' when memory is full.
 * - Selects a victim frame based on 'mode'
 * - Evicts the resident page (if any) and places 'vpn' into that frame
 * - Returns metadata about the evicted page
 * - Writes the PFN used for the new page into *new_frame (if non-NULL)
 */
evicted_page replacePage(mmu *mmu, int vpn, enum repl mode, int *new_frame);

#endif // MEMSIM_H
