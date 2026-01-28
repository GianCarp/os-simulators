#ifndef POLICIES_H
#define POLICIES_H

#include "schedsim.h"

/*
 * Scheduling policy implementations for schedsim.
 *
 * Each policy updates job_t fields such as start_time and finish_time.
 * Assumes jobs[] sorted by arrival_time, start_time is initialised to -1 and
 * remaining_time to run_time.
 */

// First come first serve, non-pre-emptive
void fcfs(job_t *jobs, const int num_jobs);

// Shortest job first, non-pre-emptive
void sjf(job_t *jobs, const int num_jobs);

// Round robin with fixed time slice, pre-emptive.
void rr(job_t *jobs, const int num_jobs, const int time_slice);

#endif // POLICIES_H