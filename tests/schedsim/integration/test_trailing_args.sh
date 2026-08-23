#!/bin/bash

# A single policy takes an exact argument count: 3 for FCFS or SJF, 5 for RR
# with its --time-slice pair. Anything past that is meaningless, and meaningless
# input has to be rejected rather than ignored.
set -e

source "$(dirname "$0")/../helpers.sh"

# A real flag, but FCFS has no use for a time slice
assert_exit_code $EXIT_USAGE "$WORKLOADS/two_jobs.txt" FCFS --time-slice 4

# Trailing arguments that mean nothing at all.
assert_exit_code $EXIT_USAGE "$WORKLOADS/two_jobs.txt" SJF junk
assert_exit_code $EXIT_USAGE "$WORKLOADS/two_jobs.txt" SJF junk extra

echo "PASS: test_trailing_args"
