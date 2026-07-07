#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

"$SCRIPT_DIR/memsim/run_memsim_tests.sh"
# "$SCRIPT_DIR/schedsim/run_schedsim_tests.sh"  # uncomment later
