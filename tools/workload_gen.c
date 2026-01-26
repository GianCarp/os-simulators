// Generate schedsim workloads in format: <id> <arrival_time> <run_time>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void usage(const char *prog) {
  fprintf(stderr,
          "Usage:\n"
          "  %s -n N [options] > workload.txt\n"
          "\n"
          "Required:\n"
          "  -n N                 Number of jobs\n"
          "\n"
          "Options:\n"
          "      --seed S         RNG seed (default: time)\n"
          "      --max-arrival T  Max arrival time (default: 50)\n"
          "      --min-run R      Min run time (default: 1)\n"
          "      --max-run R      Max run time (default: 20)\n"
          "      --arrival {uniform|burst}\n"
          "      --burst-prob P   Burst probability in percent (default: 35)\n"
          "      -h               Show this help\n",
          prog);
}

static int parse_int(const char *s, int *out) {
  char *end = NULL;
  long v = strtol(s, &end, 10);
  if (!s || *s == '\0' || *end != '\0') {
    return -1;
  }
  *out = (int)v;
  return 0;
}

static int clamp_int(int v, int lo, int hi) {
  if (v < lo) {
    return lo;
  }
  if (v > hi) {
    return hi;
  }
  return v;
}

static int rand_inclusive(int lo, int hi) {
  // assumes lo <= hi
  int span = hi - lo + 1;
  return lo + (rand() % span);
}

typedef struct {
  int id;
  int arrival;
  int run;
} job_row_t;

int main(int argc, char **argv) {
  int num_jobs = -1;
  int seed = (int)time(NULL);
  int max_arrival = 50;
  int min_run = 1;
  int max_run = 20;
  int burst_prob = 35; // percent chance of same arrival in burst mode

  enum { ARR_UNIFORM, ARR_BURST } arrival_mode = ARR_BURST;

  for (int i = 1; i < argc; i++) {
    const char *arg = argv[i];

    if (strcmp(arg, "-n") == 0) {
      if (++i >= argc || parse_int(argv[i], &num_jobs) != 0) {
        fprintf(stderr, "Bad -n value\n");
        return 1;
      }

    } else if (strcmp(arg, "--seed") == 0) {
      if (++i >= argc || parse_int(argv[i], &seed) != 0) {
        fprintf(stderr, "Bad --seed\n");
        return 1;
      }

    } else if (strcmp(arg, "--max-arrival") == 0) {
      if (++i >= argc || parse_int(argv[i], &max_arrival) != 0) {
        fprintf(stderr, "Bad --max-arrival\n");
        return 1;
      }

    } else if (strcmp(arg, "--min-run") == 0) {
      if (++i >= argc || parse_int(argv[i], &min_run) != 0) {
        fprintf(stderr, "Bad --min-run\n");
        return 1;
      }

    } else if (strcmp(arg, "--max-run") == 0) {
      if (++i >= argc || parse_int(argv[i], &max_run) != 0) {
        fprintf(stderr, "Bad --max-run\n");
        return 1;
      }

    } else if (strcmp(arg, "--arrival") == 0) {
      if (++i >= argc) {
        fprintf(stderr, "Missing --arrival value\n");
        return 1;
      }

      if (strcmp(argv[i], "uniform") == 0) {
        arrival_mode = ARR_UNIFORM;
      } else if (strcmp(argv[i], "burst") == 0) {
        arrival_mode = ARR_BURST;
      } else {
        fprintf(stderr, "Unknown arrival mode\n");
        return 1;
      }

    } else if (strcmp(arg, "--burst-prob") == 0) {
      if (++i >= argc || parse_int(argv[i], &burst_prob) != 0) {
        fprintf(stderr, "Bad --burst-prob\n");
        return 1;
      }
      burst_prob = clamp_int(burst_prob, 0, 100);

    } else if (strcmp(arg, "-h") == 0) {
      usage(argv[0]);
      return 0;

    } else {
      fprintf(stderr, "Unknown option: %s\n", arg);
      usage(argv[0]);
      return 1;
    }
  }

  if (num_jobs <= 0) {
    usage(argv[0]);
    return 1;
  }
  if (max_arrival < 0) {
    fprintf(stderr, "--max-arrival must be >= 0\n");
    return 1;
  }
  if (min_run <= 0 || max_run <= 0 || min_run > max_run) {
    fprintf(stderr, "Invalid run-time bounds\n");
    return 1;
  }

  srand((unsigned)seed);

  job_row_t *rows = (job_row_t *)malloc((size_t)num_jobs * sizeof(job_row_t));
  if (!rows) {
    fprintf(stderr, "Out of memory\n");
    return 1;
  }

  int current_arrival = 0;

  for (int j = 0; j < num_jobs; j++) {
    int id = j + 1;
    int arrival = 0;

    if (arrival_mode == ARR_UNIFORM) {
      arrival = rand_inclusive(0, max_arrival);
    } else {
      // bursty arrivals: often same time, occasionally jump forward
      if (j == 0) {
        current_arrival = rand_inclusive(0, max_arrival);
      } else {
        int r = rand_inclusive(0, 99);
        if (r >= burst_prob) {
          int step = rand_inclusive(1, 3);
          current_arrival += step;
          if (current_arrival > max_arrival)
            current_arrival = max_arrival;
        }
      }
      arrival = current_arrival;
    }

    int run = rand_inclusive(min_run, max_run);

    rows[j].id = id;
    rows[j].arrival = arrival;
    rows[j].run = run;
  }

  for (int j = 0; j < num_jobs; j++) {
    printf("%d %d %d\n", rows[j].id, rows[j].arrival, rows[j].run);
  }

  free(rows);
  return 0;
}
