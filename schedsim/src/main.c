#include "args.h"
#include "schedsim.h"

int main(int argc, char *argv[]) {
  sched_config_t cfg;

  enum exit_code rc = parse_args(argc, argv, &cfg);
  if (rc != EXIT_OK) {
    return rc;
  }

  if (cfg.help) {
    return EXIT_OK;
  }

  workload_t w;
  rc = workload_load(cfg.workload_path, &w);
  if (rc != EXIT_OK) {
    return rc;
  }

  for (int i = 0; i < cfg.num_policies; i++) {
    schedsim_run(&w, cfg.policies[i], cfg.time_slice);
  }

  workload_free(&w);

  return EXIT_OK;
}
