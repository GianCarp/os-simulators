#include "args.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *prog) {
  fprintf(stderr,
          "Usage:\n"
          "  %s <inputfile> <policy> [--time-slice N]\n"
          "  %s <inputfile> --all --time-slice N\n"
          "  %s <inputfile> --policies P1,P2[,P3...] [--time-slice N]\n"
          "\n"
          "Policies: FCFS, SJF, RR\n"
          "\n"
          "Options:\n"
          "  --time-slice N   Required when RR is selected\n"
          "  -h               Show this help message\n",
          prog, prog, prog);
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

int parse_args(int argc, char *argv[], sched_config_t *cfg) {
  cfg->workload_path = NULL;
  cfg->num_policies = 0;
  cfg->time_slice = -1;
  cfg->help = 0;

  if (argc >= 2 && (strcmp(argv[1], "-h") == 0)) {
    usage(argv[0]);
    cfg->help = 1;
    return 0;
  }

  if (argc < 3 || argc > 6) {
    usage(argv[0]);
    return -1;
  }

  cfg->workload_path = argv[1];

  if (strcmp(argv[2], "--all") == 0) {
    if (argc != 5 || strcmp(argv[3], "--time-slice") != 0) {
      usage(argv[0]);
      return -1;
    }

    cfg->time_slice = atoi(argv[4]);
    if (cfg->time_slice <= 0) {
      usage(argv[0]);
      return -1;
    }

    cfg->policies[0] = FCFS;
    cfg->policies[1] = SJF;
    cfg->policies[2] = RR;
    cfg->num_policies = 3;

  } else if (strcmp(argv[2], "--policies") == 0) {

    if (argc != 4 && argc != 6) {
      usage(argv[0]);
      return -1;
    }

    const char *list = argv[3];

    if (parse_policy_list(list, cfg->policies, &cfg->num_policies) != 0) {
      usage(argv[0]);
      return -1;
    }

    // After parsing, check whether RR is included
    int includes_rr = 0;
    for (int i = 0; i < cfg->num_policies; i++) {
      if (cfg->policies[i] == RR) {
        includes_rr = 1;
        break;
      }
    }

    if (includes_rr) {
      if (argc != 6 || strcmp(argv[4], "--time-slice") != 0) {
        fprintf(stderr, "RR selected: must supply --time-slice N\n");
        usage(argv[0]);
        return -1;
      }

      cfg->time_slice = atoi(argv[5]);
      if (cfg->time_slice <= 0) {
        fprintf(stderr, "--time-slice must be > 0\n");
        usage(argv[0]);
        return -1;
      }
    } else {
      // No RR, time_slice not needed
      cfg->time_slice = -1;
      if (argc == 6) {
        fprintf(stderr, "--time-slice provided but RR not selected\n");
        usage(argv[0]);
        return -1;
      }
    }

  } else {
    // Single policy
    if (strcmp(argv[2], "FCFS") == 0) {
      cfg->policies[0] = FCFS;
      cfg->num_policies = 1;

    } else if (strcmp(argv[2], "SJF") == 0) {
      cfg->policies[0] = SJF;
      cfg->num_policies = 1;

    } else if (strcmp(argv[2], "RR") == 0) {
      if (argc != 5 || strcmp(argv[3], "--time-slice") != 0) {
        usage(argv[0]);
        return -1;
      }

      cfg->time_slice = atoi(argv[4]);
      if (cfg->time_slice <= 0) {
        usage(argv[0]);
        return -1;
      }

      cfg->policies[0] = RR;
      cfg->num_policies = 1;

    } else {
      usage(argv[0]);
      return -1;
    }
  }

  return 0;
}
