#ifndef SCHEDSIM_H
#define SCHEDSIM_H

#define MAX_POLICIES 3

// Represents a single job
typedef struct {
  int id; // job identifier read from workload file, expected to be sequential
  int arrival_time; // time at which the job arrives
  int run_time;     // total CPU time required

  // Fields populated during simulation
  int start_time;     // time job is first scheduled
  int finish_time;    // time the job completes
  int remaining_time; // used for pre-emptive schedulers
} job_t;

// An entire workload from a file
typedef struct {
  job_t *jobs;  // dynamically allocated array of jobs
  int num_jobs; // number of jobs
} workload_t;

enum sched_policy { FCFS, SJF, RR };

// Load a workload from a file, expected format: <id> <arrival_time> <run_time>.
// Returns 0 on success, -1 on failure.
int workload_load(const char *filename, workload_t *workload);

// Free memory associated with a workload
void workload_free(workload_t *workload);

// Run a scheduling simulation.
// The input workload is not modified, schedsim_run operates on an internal
// copy of the jobs and reports results based on that copy.
void schedsim_run(const workload_t *workload, enum sched_policy policy,
                  int time_slice);

#endif // SCHEDSIM_H
