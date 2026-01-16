#include "memsim.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Page size fixed at 4 KB => 12-bit offset
static const int pageoffset = 12;

// Global simulator state
static int numFrames;

static int *page_table; // map VPN -> PFN, -1 if not resident
static frame_entry
    *frame_data;      // contain meta data regarding acces time, dirty bit, etc
static int _time = 0; // counter for LRU timestamps
static int clock_hand = 0; // hand position for clock algorithm
static int fifo_hand = 0;  // hand position for FIFO algorithm
static int next_frame = 0; // next free frame to allocate (sequential)

int createMMU(int frames) {
  numFrames = frames;

  page_table = (int *)malloc(NUM_PAGES * sizeof(int));
  frame_data = (frame_entry *)calloc(frames, sizeof(frame_entry));

  if (!page_table || !frame_data) {
    return -1;
  }

  for (int i = 0; i < NUM_PAGES; i++) {
    page_table[i] = -1;
  }
  for (int f = 0; f < frames; f++) {
    frame_data[f].vpn = -1;
  }

  return 0;
}

// Checks if the page is in memory, returns frame no or -1 if not found
int checkInMemory(int vpn) {
  int result = page_table[vpn];

  if (result != -1) {
    frame_data[result].access_time = _time++; // LRU
    frame_data[result].ref = 1;               // clock
  }
  return result;
}

// Allocate the next free frame to 'vpn' (only valid while free frames remain).
int allocateFrame(int vpn) {
  int pfn = next_frame++;
  page_table[vpn] = pfn;

  frame_data[pfn].vpn = vpn;
  frame_data[pfn].dirty = 0;
  frame_data[pfn].access_time = _time++;
  frame_data[pfn].ref = 1;

  return pfn;
}

// Selects a page for eviction according to the replacement algorithm
evicted_page replacePage(int vpn, enum repl mode, int *new_frame) {
  evicted_page victim;
  int victim_frame = -1; // Initialise to -1 as a gaurd

  // LRU
  if (mode == lru) {
    int oldest = frame_data[0].access_time;
    victim_frame = 0;
    // loop through all frames to find the one that was accessed least recently
    for (int i = 1; i < numFrames; i++) {
      if (frame_data[i].access_time < oldest) {
        oldest = frame_data[i].access_time;
        victim_frame = i;
      }
    }

    // random
  } else if (mode == _random) {
    // unbiased random selection in [0, numFrames)
    unsigned span = (unsigned)numFrames;
    unsigned limit = (RAND_MAX + 1u) - ((RAND_MAX + 1u) % span);
    unsigned r;

    do {
      r = rand();
    } while (r >= limit);
    victim_frame = r % span;

    // FIFO
  } else if (mode == fifo) {
    victim_frame = fifo_hand;
    fifo_hand = (fifo_hand + 1) % numFrames;

    // clock
  } else if (mode == _clock) {
    while (frame_data[clock_hand].ref == 1) {
      frame_data[clock_hand].ref = 0;
      clock_hand = (clock_hand + 1) % numFrames;
    }
    victim_frame = clock_hand;
    clock_hand = (clock_hand + 1) % numFrames;

    // clean clock
  } else if (mode == _clock_clean) {
    int scanned = 0;

    // Pass 1: prefer non-dirty pages
    while (scanned < numFrames) {
      if (frame_data[clock_hand].ref == 1) {
        frame_data[clock_hand].ref = 0;
      } else if (frame_data[clock_hand].ref == 0 &&
                 frame_data[clock_hand].dirty == 0) {
        victim_frame = clock_hand;
        clock_hand = (clock_hand + 1) % numFrames;
        break;
      }
      clock_hand = (clock_hand + 1) % numFrames;
      scanned++;
    }

    // Pass 2: if no clean victim found, evict first ref = 0
    if (victim_frame < 0) {
      while (frame_data[clock_hand].ref == 1) {
        frame_data[clock_hand].ref = 0;
        clock_hand = (clock_hand + 1) % numFrames;
      }
      victim_frame = clock_hand;
      clock_hand = (clock_hand + 1) % numFrames;
    }
  }
  // Defend against the case where victim_frame has not been changed from -1
  // or has been set to an invalid value
  if (victim_frame < 0) {
    fprintf(stderr, "[BUG] replacePage(): victim_frame not set (mode=%d)\n",
            mode);
    sleep(5);
    exit(EXIT_FAILURE);
  }

  // Victim page return value set-up
  victim.vpn = frame_data[victim_frame].vpn;
  victim.dirty = frame_data[victim_frame].dirty;

  // Flag victim as not being present in memory
  if (victim.vpn != -1) {
    page_table[victim.vpn] = -1;
  }

  // Map new page into victim frame
  page_table[vpn] = victim_frame;
  frame_data[victim_frame].vpn = vpn;

  // Reset metadata for the frame
  frame_data[victim_frame].access_time = _time++;
  frame_data[victim_frame].dirty = 0;
  frame_data[victim_frame].ref = 1;

  if (new_frame) {
    *new_frame = victim_frame;
  }

  return victim;
}

int main(int argc, char *argv[]) {

  char *tracename;   // path to tracefile
  uint32_t address;  // full address, shift by 12 bits to get the vpn
  int vpn;           // vpn derived from address
  int pfn;           // pfn the page currently resides in
  int status;        // captures return status of createMMU(), -1 indicates heap
                     // allocation failed.
  int memory_access; // stores the return value of fscanf(), should be 2 to
                     // indicate an address and R/W instruction
  char rw;

  int no_events = 0;
  int disk_writes = 0;
  int page_faults = 0;
  int debugmode;
  enum repl replace;
  int allocated =
      0; // stores how many frames have been allocated so far, before memory is
         // full can just call allocateFrame(), after need to call replacePage()

  evicted_page Pvictim;
  FILE *trace;
  unsigned seed = 1;

  if (argc < 5) {
    printf("Usage: ./memsim inputfile numberframes replacementmode debugmode "
           "[seed]\n");
    return -1;
  }

  // Argument parsing begins
  tracename = argv[1];
  trace = fopen(tracename, "r");
  if (trace == NULL) {
    printf("Cannot open trace file %s\n", tracename);
    return -1;
  }

  int frames = atoi(argv[2]);
  if (frames < 1) {
    printf("Frame number must be at least 1\n");
    return -1;
  }

  if (strcmp(argv[3], "lru") == 0) {
    replace = lru;
  } else if (strcmp(argv[3], "rand") == 0) {
    replace = _random;
  } else if (strcmp(argv[3], "clock") == 0) {
    replace = _clock;
  } else if (strcmp(argv[3], "fifo") == 0) {
    replace = fifo;
  } else if (strcmp(argv[3], "clean-clock") == 0) {
    replace = _clock_clean;
  } else {
    printf("Replacement algorithm must be rand, fifo, lru, clock or "
           "clean-clock\n");
    return -1;
  }

  if (strcmp(argv[4], "quiet") == 0)
    debugmode = 0;
  else if (strcmp(argv[4], "debug") == 0)
    debugmode = 1;
  else {
    printf("Debug mode must be quiet or debug\n");
    return -1;
  }

  if (argc >= 6) {
    seed = (unsigned)strtoul(argv[5], NULL, 10);
  }

  srand(seed);

  // Argument parsing ends

  status = createMMU(frames);
  if (status == -1) {
    printf("Cannot create MMU\n");
    return -1;
  }

  memory_access = fscanf(trace, "%x %c", &address, &rw);
  while (memory_access == 2) {
    vpn = (int)(address >> pageoffset);
    pfn = checkInMemory(vpn);

    if (pfn == -1) {
      page_faults++;
      if (debugmode) {
        printf("Page fault %8d\n", vpn);
      }

      if (allocated < frames) {
        pfn = allocateFrame(vpn);
        allocated++;
      } else {
        Pvictim = replacePage(vpn, replace, &pfn);

        if (Pvictim.dirty) {
          disk_writes++;
          if (debugmode)
            printf("Disk write %8d\n", Pvictim.vpn);
        } else {
          if (debugmode)
            printf("Discard    %8d\n", Pvictim.vpn);
        }
      }
    }

    // Marking the frame as dirty if written to
    if (pfn != -1 && rw == 'W') {
      frame_data[pfn].dirty = 1;
    }

    if (rw != 'R' && rw != 'W') {
      printf("Badly formatted file. Error on line %d\n", no_events + 1);
      return -1;
    }

    if (debugmode) {
      if (rw == 'R') {
        printf("reading    %8d\n", vpn);
      } else { // rw == 'W'
        printf("writing    %8d\n", vpn);
      }
    }

    no_events++;
    memory_access = fscanf(trace, "%x %c", &address, &rw);
  }

  printf("total memory frames:  %d\n", frames);
  printf("events in trace:      %d\n", no_events);
  printf("total disk reads:     %d\n", page_faults);
  printf("total disk writes:    %d\n", disk_writes);
  printf("page fault rate:      %.4f\n", (float)page_faults / no_events);
  printf("seed:                %u\n", seed);

  free(page_table);
  free(frame_data);
  fclose(trace);

  return 0;
}
