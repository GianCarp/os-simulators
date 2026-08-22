#!/bin/bash

# The worked example in schedsim/README.md is generated from
# schedsim/workloads/example.txt with --all --time-slice 4. Asserting the
# published figures keeps the README honest and exercises the per-policy stat
# extraction, since --all prints three blocks sharing the same stat labels.

set -e

source "$(dirname "$0")/../helpers.sh"

run_schedsim "$REAL_WORKLOADS/example.txt" --all --time-slice 4

assert_stat FCFS "Response time"   34.90
assert_stat FCFS "Turnaround time" 43.20
assert_stat FCFS "Waiting time"    34.90
assert_stat FCFS "Makespan"        93

assert_stat SJF  "Response time"   19.90
assert_stat SJF  "Turnaround time" 28.20
assert_stat SJF  "Waiting time"    19.90
assert_stat SJF  "Makespan"        93

assert_stat RR   "Response time"   14.80
assert_stat RR   "Turnaround time" 50.30
assert_stat RR   "Waiting time"    42.00
assert_stat RR   "Makespan"        93

echo "PASS: test_golden_example"
