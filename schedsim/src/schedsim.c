#include "schedsim.h"
#include "policies.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MAX_POLICIES = 3 };

static void usage(const char *prog) {
  fprintf(stderr,
          "Usage:\n"
          "  %s <inputfile> <policy> [--time-slice N]\n"
          "  %s <inputfile> --all --time-slice N\n"
          "  %s <inputfile> --policies P1,P2[,P3...] [--time-slice N]\n"
          "\n"
          "Policies: FCFS, SJF, RR\n"
          "Note: if RR or all is selected, you must provide --time-slice N\n",
          prog, prog, prog);
}
static const char *policy_name(enum sched_policy policy) {
  switch (policy) {
  case FCFS:
    return "FCFS";
  case SJF:
    return "SJF";
  case RR:
    return "RR";
  default:
    return "UNKNOWN";
  }
}

static void print_results(const job_t *jobs, int num_jobs,
                          enum sched_policy policy) {

  const char *pol = policy_name(policy);

  printf("\n===== Policy: %s =====\n", pol);
  printf(" id | arrival | run | start | finish | response | turnaround | "
         "waiting\n");
  printf("----+---------+-----+-------+--------+----------+------------+-------"
         "-\n");

  int total_response = 0;
  int total_turnaround = 0;
  int total_waiting = 0;
  int makespan = 0;

  for (int i = 0; i < num_jobs; i++) {
    const job_t *j = &jobs[i];

    int response = j->start_time - j->arrival_time;
    int turnaround = j->finish_time - j->arrival_time;
    int waiting = turnaround - j->run_time;

    total_response += response;
    total_turnaround += turnaround;
    total_waiting += waiting;

    if (j->finish_time > makespan) {
      makespan = j->finish_time;
    }

    printf(" %2d | %7d | %3d | %5d | %6d | %8d | %10d | %6d\n", j->id,
           j->arrival_time, j->run_time, j->start_time, j->finish_time,
           response, turnaround, waiting);
  }

  printf("\nAverages:\n");
  printf("  Response time   : %.2f\n", (double)total_response / num_jobs);
  printf("  Turnaround time : %.2f\n", (double)total_turnaround / num_jobs);
  printf("  Waiting time    : %.2f\n", (double)total_waiting / num_jobs);
  printf("  Makespan        : %d\n", makespan);
}

static int parse_policy_list(const char *list, enum sched_policy *policies,
                             int *num_policies) {
  char buf[128];

  if (strlen(list) >= sizeof(buf)) {
    fprintf(stderr, "Policy list too long\n");
    return -1;
  }

  strcpy(buf, list);

  *num_policies = 0;

  char *tok = strtok(buf, ",");

  while (tok != NULL) {
    if (*tok == '\0') {
      fprintf(stderr, "Empty policy in list\n");
      return -1;
    }

    if (*num_policies >= MAX_POLICIES) {
      fprintf(stderr, "Too many policies (max 3)\n");
      return -1;
    }

    if (strcmp(tok, "FCFS") == 0) {
      policies[(*num_policies)++] = FCFS;
    } else if (strcmp(tok, "SJF") == 0) {
      policies[(*num_policies)++] = SJF;
    } else if (strcmp(tok, "RR") == 0) {
      policies[(*num_policies)++] = RR;
    } else {
      fprintf(stderr, "Unknown policy in list: %s\n", tok);
      return -1;
    }

    tok = strtok(NULL, ",");
  }

  if (*num_policies == 0) {
    fprintf(stderr, "No policies specified\n");
    return -1;
  }

  return 0;
}

void workload_free(workload_t *workload) {
  free(workload->jobs);
  workload->jobs = NULL;
  workload->num_jobs = 0;
}

// allocate memory up front for the number of jobs to simulate, re-size as
// necessary.
static int populate_workload(FILE *work, workload_t *workload) {

  int capacity = 16;
  workload->jobs = (job_t *)malloc(capacity * sizeof(job_t));
  if (workload->jobs == NULL) {
    fprintf(stderr, "Failed to allocate memory in populate_workload().\n");
    return -1;
  }

  workload->num_jobs = 0;
  // parse into signed types so negative values are detectable
  int id;
  int arrival_time;
  int run_time;

  int rc;

  while ((rc = fscanf(work, "%d %d %d", &id, &arrival_time, &run_time)) == 3) {

    if (id != (workload->num_jobs + 1)) {
      fprintf(stderr, "Bad data: expected id %d but got %d\n",
              workload->num_jobs + 1, id);
      workload_free(workload);
      return -1;
    }
    if (arrival_time < 0) {
      fprintf(stderr, "Bad data: arrival_time must be >= 0 (id = %d)\n", id);
      workload_free(workload);
      return -1;
    }
    if (run_time <= 0) {
      fprintf(stderr, "Bad data: run_time must be > 0 (id = %d)\n", id);
      workload_free(workload);
      return -1;
    }

    if (workload->num_jobs == capacity) {
      capacity *= 2;
      job_t *temp = (job_t *)realloc(workload->jobs, capacity * sizeof(job_t));
      if (temp == NULL) {
        fprintf(stderr, "Failed to allocate memory in populate_workload().\n");
        workload_free(workload);
        return -1;
      }
      workload->jobs = temp;
    }

    job_t current_job;
    current_job.id = id;
    current_job.arrival_time = arrival_time;
    current_job.run_time = run_time;
    workload->jobs[workload->num_jobs++] = current_job;
  }

  if (rc != EOF) {
    fprintf(stderr, "Bad data.\n");
    workload_free(workload);
    return -1;
  }
  if (workload->num_jobs == 0) {
    fprintf(stderr, "No jobs found in workload.\n");
    workload_free(workload);
    return -1;
  }

  return 0;
}

static int compare_jobs_by_arrival(const void *jb1, const void *jb2) {
  const job_t *job_1 = (const job_t *)jb1;
  const job_t *job_2 = (const job_t *)jb2;

  if (job_1->arrival_time < job_2->arrival_time) {
    return -1;
  }
  if (job_1->arrival_time > job_2->arrival_time) {
    return 1;
  }

  // Tie-break on id
  if (job_1->id < job_2->id) {
    return -1;
  }
  if (job_1->id > job_2->id) {
    return 1;
  }

  return 0;
}

// Load a workload from a file, expected format: <id> <arrival_time> <run_time>.
// Returns 0 on success, -1 on failure.
int workload_load(const char *filename, workload_t *workload) {

  workload->jobs = NULL;
  workload->num_jobs = 0;

  FILE *work = fopen(filename, "r");
  if (work == NULL) {
    fprintf(stderr, "Cannot open file %s\n", filename);
    return -1;
  }

  if (populate_workload(work, workload) != 0) {
    fclose(work);
    fprintf(stderr, "Call to workload_load() failed.\n");
    return -1;
  }
  fclose(work);
  qsort(workload->jobs, workload->num_jobs, sizeof(job_t),
        compare_jobs_by_arrival);

  return 0;
}

static job_t *copy_jobs(const workload_t *workload) {

  job_t *jobs = (job_t *)malloc(workload->num_jobs * sizeof(job_t));
  if (jobs == NULL) {
    return NULL;
  }

  memcpy(jobs, workload->jobs, workload->num_jobs * sizeof(job_t));

  for (int i = 0; i < workload->num_jobs; i++) {
    jobs[i].start_time = -1;
    jobs[i].finish_time = -1;
    jobs[i].remaining_time = jobs[i].run_time;
  }
  return jobs;
}

static void run_policy(job_t *jobs, enum sched_policy policy,
                       const int num_jobs, int time_slice) {

  if (policy == FCFS) {
    fcfs(jobs, num_jobs);
  } else if (policy == SJF) {
    sjf(jobs, num_jobs);
  } else if (policy == RR) {
    rr(jobs, num_jobs, time_slice);
  }
}

void schedsim_run(const workload_t *workload, enum sched_policy policy,
                  int time_slice) {
  job_t *jobs = copy_jobs(workload); // per-run mutable state
  if (jobs == NULL) {
    fprintf(stderr, "Failed to allocate job copy\n");
    return;
  }
  int num_jobs = workload->num_jobs;

  run_policy(jobs, policy, num_jobs, time_slice);

  print_results(jobs, workload->num_jobs, policy); // pure reporting

  free(jobs);
}

int main(int argc, char *argv[]) {

  //
  // BEGIN PARSING ARGUMENTS
  //

  if (argc < 3 || argc > 6) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  int time_slice = -1;
  enum sched_policy policies[MAX_POLICIES];
  int num_policies = 0;

  if (strcmp(argv[2], "--all") == 0) {
    if (argc != 5 || strcmp(argv[3], "--time-slice") != 0) {
      usage(argv[0]);
      return EXIT_FAILURE;
    }

    time_slice = atoi(argv[4]);
    if (time_slice <= 0) {
      usage(argv[0]);
      return EXIT_FAILURE;
    }

    policies[0] = FCFS;
    policies[1] = SJF;
    policies[2] = RR;
    num_policies = 3;

  } else if (strcmp(argv[2], "--policies") == 0) {

    /* Fixed-order supported form examples:
     *   schedsim <workload> --policies FCFS,SJF -> argc = 4
     *   schedsim <workload> --policies FCFS,RR,SJF --time-slice N -> argc = 6
     *   i.e if RR included must give --time-slice N
     */
    if (argc != 4 && argc != 6) {
      usage(argv[0]);
      return EXIT_FAILURE;
    }

    const char *list = argv[3];

    if (parse_policy_list(list, policies, &num_policies) != 0) {
      usage(argv[0]);
      return EXIT_FAILURE;
    }

    // After parsing, check whether RR is included
    int includes_rr = 0;
    for (int i = 0; i < num_policies; i++) {
      if (policies[i] == RR) {
        includes_rr = 1;
        break;
      }
    }

    if (includes_rr) {
      if (argc != 6 || strcmp(argv[4], "--time-slice") != 0) {
        fprintf(stderr, "RR selected: must supply --time-slice N\n");
        usage(argv[0]);
        return EXIT_FAILURE;
      }

      time_slice = atoi(argv[5]);
      if (time_slice <= 0) {
        fprintf(stderr, "--time-slice must be > 0\n");
        return EXIT_FAILURE;
      }
    } else {
      // No RR, time_slice not needed
      time_slice = -1;
      if (argc == 6) {
        fprintf(stderr, "--time-slice provided but RR not selected\n");
        usage(argv[0]);
        return EXIT_FAILURE;
      }
    }

  } else {
    // Single policy
    if (strcmp(argv[2], "FCFS") == 0) {
      policies[0] = FCFS;
      num_policies = 1;

    } else if (strcmp(argv[2], "SJF") == 0) {
      policies[0] = SJF;
      num_policies = 1;

    } else if (strcmp(argv[2], "RR") == 0) {
      if (argc != 5 || strcmp(argv[3], "--time-slice") != 0) {
        usage(argv[0]);
        return EXIT_FAILURE;
      }

      time_slice = atoi(argv[4]);
      if (time_slice <= 0) {
        usage(argv[0]);
        return EXIT_FAILURE;
      }

      policies[0] = RR;
      num_policies = 1;

    } else {
      usage(argv[0]);
      return EXIT_FAILURE;
    }
  }

  //
  // END PARSING ARGUMENTS
  //

  workload_t w;
  if (workload_load(argv[1], &w) != 0) {
    return EXIT_FAILURE;
  }
  for (int i = 0; i < num_policies; i++) {
    schedsim_run(&w, policies[i], time_slice);
  }

  workload_free(&w);

  return EXIT_SUCCESS;
}