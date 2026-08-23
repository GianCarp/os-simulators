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

// Numbering starts at 2 because 1 is reserved for an unexpected internal
// failure: it is what an assert() in policies.c or queue.c produces, and what
// a caller gets from a crash. Reserving it keeps those from colliding with a
// real code. Unlike memsim this enum lives in the header, because args.c and
// schedsim.c both return codes from it.
enum exit_code {
  EXIT_OK = 0,
  EXIT_USAGE = 2,           // argc outside [3, 6], or trailing arguments
  EXIT_BAD_POLICY = 3,      // unknown policy name, empty or oversized list
  EXIT_TIME_SLICE = 4,      // RR selected, slice missing or not a positive int
  EXIT_WORKLOAD_OPEN = 5,   // workload file could not be opened
  EXIT_WORKLOAD_FORMAT = 6, // malformed line, id out of sequence, bad values
  EXIT_WORKLOAD_EMPTY = 7,  // file parsed but described no jobs
  EXIT_NO_MEMORY = 8,       // allocation failed
};

// Load a workload from a file, expected format: <id> <arrival_time> <run_time>.
// Returns EXIT_OK, or the code identifying why the file was rejected.
enum exit_code workload_load(const char *filename, workload_t *workload);

// Free memory associated with a workload
void workload_free(workload_t *workload);

// Run a scheduling simulation.
// The input workload is not modified, schedsim_run operates on an internal
// copy of the jobs and reports results based on that copy.
void schedsim_run(const workload_t *workload, enum sched_policy policy,
                  int time_slice);

#endif // SCHEDSIM_H
