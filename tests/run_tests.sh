#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Every runner is given a chance to run, so one simulator's failure does not
# hide the other's results. Without the explicit aggregation the exit status
# would be that of the last runner alone.
status=0

"$SCRIPT_DIR/memsim/run_memsim_tests.sh" || status=1
"$SCRIPT_DIR/schedsim/run_schedsim_tests.sh" || status=1

exit $status
