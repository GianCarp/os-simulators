#include "policies.h"
#include "queue.h"
#include <string.h>

void fcfs(job_t *jobs, const int num_jobs) {
  int current_time = 0;

  for (int i = 0; i < num_jobs; i++) {
    if (current_time < jobs[i].arrival_time) {
      current_time = jobs[i].arrival_time; // CPU idle until next arrival
    }

    jobs[i].start_time = current_time;
    jobs[i].finish_time = current_time + jobs[i].run_time;
    jobs[i].remaining_time = 0;

    current_time = jobs[i].finish_time;
  }
}

void sjf(job_t *jobs, const int num_jobs) {

  int current_time = 0;
  int completed = 0;

  int done[num_jobs];
  memset(done, 0, sizeof(done));

  while (completed < num_jobs) {
    int best = -1;

    // Find shortest job
    for (int i = 0; i < num_jobs; i++) {
      if (done[i] || jobs[i].arrival_time > current_time) {
        continue;
      }
      // set best to first job that can be run. For ties use earliest arrival,
      // and then lower id
      if (best == -1) {
        best = i;
      }
      // compare other jobs to current best
      else {
        if (jobs[i].run_time < jobs[best].run_time) {
          best = i;
        } else if (jobs[i].run_time == jobs[best].run_time) {
          if (jobs[i].arrival_time < jobs[best].arrival_time) {
            best = i;
          } else if (jobs[i].arrival_time == jobs[best].arrival_time) {
            if (jobs[i].id < jobs[best].id) {
              best = i;
            }
          }
        }
      }
    }

    if (best == -1) {
      // CPU idle: jump to next arrival among unfinished jobs
      int next_arrival = -1;
      for (int i = 0; i < num_jobs; i++) {
        if (done[i]) {
          continue;
        }
        int a = jobs[i].arrival_time;
        if (a > current_time && (next_arrival == -1 || a < next_arrival)) {
          next_arrival = a;
        }
      }
      // Should always find one if not completed, but guard anyway
      if (next_arrival != -1)
        current_time = next_arrival;
      continue;
    }

    jobs[best].start_time = current_time;
    jobs[best].finish_time = current_time + jobs[best].run_time;
    jobs[best].remaining_time = 0;

    current_time = jobs[best].finish_time;
    done[best] = 1;
    completed++;
  }
}

// Round Robin scheduling (preemptive). Uses a FIFO ready queue. Assumes jobs[]
// sorted by arrival_time and that start_time is initialised to -1 and
// remaining_time to run_time.
void rr(job_t *jobs, const int num_jobs, const int time_slice) {

  int current_time = 0;
  int completed = 0;

  queue_t rq;
  q_init(&rq, num_jobs);

  int next_arrival = 0; // index of next job not yet enqueued

  while (completed < num_jobs) {

    // If nothing is ready, jump time to next arrival
    if (q_empty(&rq)) {
      if (next_arrival < num_jobs &&
          current_time < jobs[next_arrival].arrival_time) {
        current_time = jobs[next_arrival].arrival_time;
      }
    }

    // Enqueue all jobs that have arrived by current_time
    while (next_arrival < num_jobs &&
           jobs[next_arrival].arrival_time <= current_time) {
      q_push(&rq, next_arrival);
      next_arrival++;
    }

    // Still nothing to run, i.e. jobs remain but all arrive later
    if (q_empty(&rq)) {
      continue;
    }

    int i = q_pop(&rq);

    // First time this job runs
    if (jobs[i].start_time == -1) {
      jobs[i].start_time = current_time;
    }

    int slice = (jobs[i].remaining_time < time_slice) ? jobs[i].remaining_time
                                                      : time_slice;

    jobs[i].remaining_time -= slice;
    current_time += slice;

    // Enqueue jobs that arrived during this time slice
    while (next_arrival < num_jobs &&
           jobs[next_arrival].arrival_time <= current_time) {
      q_push(&rq, next_arrival);
      next_arrival++;
    }

    if (jobs[i].remaining_time == 0) {
      jobs[i].finish_time = current_time;
      completed++;
    } else {
      // Not finished: put back at end of ready queue
      q_push(&rq, i);
    }
  }

  q_free(&rq);
}
