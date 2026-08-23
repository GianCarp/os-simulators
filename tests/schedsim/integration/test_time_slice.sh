#!/bin/bash

# The RR / --time-slice coupling is required when RR runs, rejected when it does
# not, and the value itself must be a positive integer.

set -e

source "$(dirname "$0")/../helpers.sh"

# Values that are not a positive integer. "4x" is here because atoi stops at the
# first non-digit and reads it as 4, so a > 0 check alone accepts it. Only
# is_uint rejects the whole token.
assert_exit_code $EXIT_TIME_SLICE "$WORKLOADS/two_jobs.txt" RR --time-slice 0
assert_exit_code $EXIT_TIME_SLICE "$WORKLOADS/two_jobs.txt" RR --time-slice -1
assert_exit_code $EXIT_TIME_SLICE "$WORKLOADS/two_jobs.txt" RR --time-slice abc
assert_exit_code $EXIT_TIME_SLICE "$WORKLOADS/two_jobs.txt" RR --time-slice 4x

# A slice supplied with no RR to apply it to.
assert_exit_code $EXIT_USAGE "$WORKLOADS/two_jobs.txt" --policies FCFS --time-slice 4

# The first test is simple RR with no time-slice provided, so EXIT_TIME_SLICE is
# the obvious code. For the other two tests, they are missing both the time slice
# and have an incorrect argc value. The choice is to report the missing time-slice.
assert_exit_code $EXIT_TIME_SLICE "$WORKLOADS/two_jobs.txt" RR
assert_exit_code $EXIT_TIME_SLICE "$WORKLOADS/two_jobs.txt" --policies SJF,RR
assert_exit_code $EXIT_TIME_SLICE "$WORKLOADS/two_jobs.txt" --all

echo "PASS: test_time_slice"
