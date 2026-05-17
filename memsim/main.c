#define _XOPEN_SOURCE 600

#include "memsim.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void usage(const char *prog) {
  fprintf(stderr,
          "Usage:\n"
          "  %s <tracefile> <num_frames> <policy> [-debug] [-seed N]\n"
          "\n"
          "Policies:\n"
          "  fifo | lru-simple | lru-advanced | rand | clock | clean-clock\n"
          "\n"
          "Options:\n"
          "  -debug      Print per-access trace (default: quiet)\n"
          "  -seed N     Set RNG seed for rand policy (default: 1)\n"
          "  -h          Show this help message\n",
          prog);
}

static int is_uint(const char *s) {
  if (*s == '\0')
    return 0;
  for (const char *p = s; *p; p++) {
    if (*p < '0' || *p > '9')
      return 0;
  }
  return 1;
}

// Page size fixed at 4 KB => 12-bit offset
static const int pageoffset = 12;

int main(int argc, char *argv[]) {

  char *tracename;   // path to tracefile
  uint32_t address;  // full address, shift by 12 bits to get the vpn
  int vpn;           // vpn derived from address
  int pfn;           // pfn the page currently resides in
  int memory_access; // stores the return value of fscanf(), should be 2 to
                     // indicate an address and R/W instruction
  char rw;

  int no_events = 0;
  int disk_writes = 0;
  int page_faults = 0;
  int debugmode = 0; // default quiet
  enum repl replace;

  replace_result page_replaced;
  FILE *trace;
  unsigned seed = 1;

  //
  // Argument parsing begins
  //

  if (argc >= 2 && strcmp(argv[1], "-h") == 0) {
    usage(argv[0]);
    return EXIT_SUCCESS;
  }

  // Min args is 4, max is 7
  if (argc < 4 || argc > 7) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  tracename = argv[1];
  trace = fopen(tracename, "r");
  if (trace == NULL) {
    fprintf(stderr, "Cannot open trace file %s\n", tracename);
    return EXIT_FAILURE;
  }

  if (!is_uint(argv[2])) {
    fprintf(stderr, "Invalid number of frames given\n");
    usage(argv[0]);
    fclose(trace);
    return EXIT_FAILURE;
  }
  int frames = atoi(argv[2]);
  if (frames < 1) {
    fprintf(stderr, "Frame number must be at least 1\n");
    fclose(trace);
    return EXIT_FAILURE;
  }

  if (strcmp(argv[3], "lru-simple") == 0) {
    replace = lru_simple;
  } else if (strcmp(argv[3], "lru-advanced") == 0) {
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
    fprintf(stderr, "Replacement algorithm must be rand, fifo, lru-simple, "
                    "lru-advanced, clock, or clean-clock\n");
    fclose(trace);
    return EXIT_FAILURE;
  }

  // Parse optional flags starting at argv[4]
  for (int i = 4; i < argc; i++) {
    if (strcmp(argv[i], "-debug") == 0) {
      debugmode = 1;
    } else if (strcmp(argv[i], "-seed") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "-seed requires a value\n");
        usage(argv[0]);
        fclose(trace);
        return EXIT_FAILURE;
      }
      if (!is_uint(argv[i + 1])) {
        fprintf(stderr, "Invalid seed given\n");
        usage(argv[0]);
        fclose(trace);
        return EXIT_FAILURE;
      }
      seed = atoi(argv[i + 1]);
      i++; // consume the seed value
    } else {
      fprintf(stderr, "Unknown option: %s\n", argv[i]);
      usage(argv[0]);
      fclose(trace);
      return EXIT_FAILURE;
    }
  }

  srand(seed);

  //
  // Argument parsing ends
  //

  mmu *mmu_ptr = create_MMU(frames, replace);
  if (mmu_ptr == NULL) {
    fprintf(stderr, "Cannot create MMU\n");
    fclose(trace);
    return EXIT_FAILURE;
  }

  memory_access = fscanf(trace, "%x %c", &address, &rw);
  while (memory_access == 2) {
    vpn = (int)(address >> pageoffset);
    pfn = check_in_memory(mmu_ptr, vpn);

    if (pfn == -1) {
      page_faults++;
      if (debugmode) {
        printf("Page fault %8d\n", vpn);
      }

      if (has_free_frames(mmu_ptr) == 1) {
        pfn = allocate_frame(mmu_ptr, vpn);
      } else {
        page_replaced = replace_page(mmu_ptr, vpn);
        pfn = page_replaced.new_pfn;
        if (page_replaced.victim.dirty) {
          disk_writes++;
          if (debugmode) {
            printf("Disk write %8d\n", page_replaced.victim.vpn);
          }
        } else {
          if (debugmode) {
            printf("Discard    %8d\n", page_replaced.victim.vpn);
          }
        }
      }
    }

    // Marking the frame as dirty if written to
    if (pfn != -1 && rw == 'W') {
      mark_dirty(mmu_ptr, pfn);
    }

    if (rw != 'R' && rw != 'W') {
      fprintf(stderr, "Badly formatted file. Error on line %d\n",
              no_events + 1);
      destroy_MMU(mmu_ptr);
      fclose(trace);
      return EXIT_FAILURE;
    }

    if (debugmode) {
      if (rw == 'R') {
        printf("reading    %8d\n", vpn);
      } else { // rw == 'W'
        printf("writing    %8d\n", vpn);
      }
      printf("\n");
      usleep(600000);
    }
    no_events++;
    memory_access = fscanf(trace, "%x %c", &address, &rw);
  }

  double page_fault_percent = 0.0;
  if (no_events > 0) {
    page_fault_percent = (double)page_faults / (double)no_events * 100.0;
  }

  printf("%-28s %10d\n", "total memory frames:", frames);
  printf("%-28s %10d\n", "events in trace:", no_events);
  printf("%-28s %10d\n", "total disk reads:", page_faults);
  printf("%-28s %10d\n", "total disk writes:", disk_writes);
  printf("%-28s %10.4f\n", "page fault rate (%):", page_fault_percent);
  printf("%-28s %10u\n", "seed:", seed);

  destroy_MMU(mmu_ptr);
  fclose(trace);

  return EXIT_SUCCESS;
}
