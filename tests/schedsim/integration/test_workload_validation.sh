#!/bin/bash

# One fixture per rule populate_workload enforces, each a distinct way a line
# can be wrong. They share EXIT_WORKLOAD_FORMAT deliberately: the code says the
# file is malformed, the message on stderr says which line and how.

set -e

source "$(dirname "$0")/../helpers.sh"

# Ids must be sequential from 1, since the loader uses position to check them.
assert_exit_code $EXIT_WORKLOAD_FORMAT "$WORKLOADS/bad_id_gap.txt" FCFS

assert_exit_code $EXIT_WORKLOAD_FORMAT "$WORKLOADS/bad_arrival_negative.txt" FCFS
assert_exit_code $EXIT_WORKLOAD_FORMAT "$WORKLOADS/bad_runtime_zero.txt" FCFS

# A line short of its third field
assert_exit_code $EXIT_WORKLOAD_FORMAT "$WORKLOADS/truncated_line.txt" FCFS

assert_exit_code $EXIT_WORKLOAD_FORMAT "$WORKLOADS/non_numeric.txt" FCFS

echo "PASS: test_workload_validation"
