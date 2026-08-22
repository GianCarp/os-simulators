#ifndef ARGS_H
#define ARGS_H

#include "schedsim.h"

/*
 * Command line parsing for schedsim.
 *
 * Split out of main.c so the parsing rules can be exercised directly, without
 * spawning a process. Nothing here touches the filesystem or runs a
 * simulation: parse_args turns argv into a sched_config_t and stops.
 */

typedef struct {
  const char *workload_path;
  enum sched_policy policies[MAX_POLICIES];
  int num_policies;
  int time_slice; // -1 when RR was not selected, so unused is distinguishable
  int help;       // -h was given; no other field is meaningful
} sched_config_t;

// Parse argv into cfg. Returns 0 on success and -1 on any usage error, having
// already written a diagnostic and the usage text to stderr. -h is a success,
// it sets cfg.help and leaves the remaining fields unset.
int parse_args(int argc, char *argv[], sched_config_t *cfg);

#endif // ARGS_H
