#!/bin/bash

# Naming a policy that does not exist, on both the single-policy and --policies
# paths, plus the two list-shape errors parse_policy_list reports.

set -e

source "$(dirname "$0")/../helpers.sh"

assert_exit_code $EXIT_BAD_POLICY "$WORKLOADS/two_jobs.txt" LIFO

# Policy names are matched with strcmp, so they are case sensitive.
assert_exit_code $EXIT_BAD_POLICY "$WORKLOADS/two_jobs.txt" fcfs

assert_exit_code $EXIT_BAD_POLICY "$WORKLOADS/two_jobs.txt" --policies FCFS,LIFO
assert_exit_code $EXIT_BAD_POLICY "$WORKLOADS/two_jobs.txt" --policies ""
assert_exit_code $EXIT_BAD_POLICY "$WORKLOADS/two_jobs.txt" --policies FCFS,SJF,FCFS,SJF

echo "PASS: test_invalid_policy"
