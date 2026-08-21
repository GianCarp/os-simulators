#!/bin/bash

# An invalid policy name should exit with the bad policy code.

set -e

source "$(dirname "$0")/../helpers.sh"

assert_exit_code $EXIT_BAD_POLICY "$REAL_TRACES/gcc.trace" 10 policy_foo

echo "PASS: test_invalid_policy"
