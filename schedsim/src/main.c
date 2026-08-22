#include "args.h"
#include "schedsim.h"
#include <stdlib.h>

int main(int argc, char *argv[]) {
  sched_config_t cfg;

  if (parse_args(argc, argv, &cfg) != 0) {
    return EXIT_FAILURE;
  }

  if (cfg.help) {
    return EXIT_SUCCESS;
  }

  workload_t w;
  if (workload_load(cfg.workload_path, &w) != 0) {
    return EXIT_FAILURE;
  }
  for (int i = 0; i < cfg.num_policies; i++) {
    schedsim_run(&w, cfg.policies[i], cfg.time_slice);
  }

  workload_free(&w);

  return EXIT_SUCCESS;
}
