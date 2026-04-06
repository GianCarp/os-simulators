#include "schedsim.h"
#include "policies.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

  run_policy(jobs, policy, workload->num_jobs, time_slice);

  print_results(jobs, workload->num_jobs, policy); // pure reporting

  free(jobs);
}
