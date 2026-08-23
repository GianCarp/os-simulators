#!/bin/bash

# fscanf("%d %d %d") treats a newline as ordinary whitespace, so a short line is
# completed from the line after it. merged_lines.txt is
#
#   1 0
#   5
#   2 3 4
#
# which fscanf reads back as jobs "1 0 5" and "2 3 4": sequential ids,
# non-negative arrivals, positive run times. Every value check passes, so
# nothing but line-oriented parsing can tell that the file does not describe
# the workload that would be simulated.

set -e

source "$(dirname "$0")/../helpers.sh"

assert_exit_code $EXIT_WORKLOAD_FORMAT "$WORKLOADS/merged_lines.txt" FCFS

echo "PASS: test_merged_lines"
