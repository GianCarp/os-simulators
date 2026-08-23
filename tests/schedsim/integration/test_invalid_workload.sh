#!/bin/bash

# Failures about the file rather than its contents. These are distinct causes:
# a file that cannot be opened and a file that opens and yields no jobs need
# different codes, or a typo in a path is indistinguishable from an empty file.

set -e

source "$(dirname "$0")/../helpers.sh"

assert_exit_code $EXIT_WORKLOAD_OPEN "$WORKLOADS/does_not_exist.txt" FCFS
assert_exit_code $EXIT_WORKLOAD_EMPTY "$WORKLOADS/empty.txt" FCFS

echo "PASS: test_invalid_workload"
