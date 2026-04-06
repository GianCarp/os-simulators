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

typedef struct {
  int new_pfn;
  evicted_page victim;
} replace_result;

// Internal simulator state. Model assumes a 4 KB page size and a 32-bit system,
// so up to 2^20 pages.
typedef struct mmu mmu;

// Replacement policies supportd by the simulator
enum repl { _random, fifo, lru, _clock, _clock_clean };

// Core simulator API

// Initialise simulator state for a machine with 'frames' physical frames.
// Returns a pointer to the allocated mmu on success, NULL on allocation
// failure.
mmu *createMMU(int frames);

// Free memory allocated by createMMU().
void destroyMMU(mmu *mmu_ptr);

// Returns PFN if 'vpn' is in memory, or -1 if not (page fault).
// On a hit, updates metadata (LRU timestamp and CLOCK ref-bit).
int checkInMemory(mmu *mmu_ptr, int vpn);

// Allocate the next free frame to 'vpn' (only valid while free frames remain).
// Returns PFN assigned to this VPN, needed so that the frame can be marked as
// dirty if appropriate.
int allocateFrame(mmu *mmu_ptr, int vpn);

// Perform page replacement for 'vpn' when memory is full.
// Selects a victim frame based on 'mode'
// Evicts the resident page (if any) and places 'vpn' into that frame
// Returns metadata about the evicted page
replace_result replacePage(mmu *mmu, int vpn, enum repl mode);

// Returns 1 if free frames remain, -1 if all frames are occupied.
int has_free_frames(mmu *mmu_ptr);

// Marks the frame at 'pfn' as dirty, indicating it has been written to.
void mark_dirty(mmu *mmu_ptr, int pfn);

#endif // MEMSIM_H
