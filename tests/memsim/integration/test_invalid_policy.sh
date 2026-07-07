#!/bin/bash

# An invalid policy name should print the appropriate error message.

set -e

source "$(dirname "$0")/../helpers.sh"

assert_exit_code 1 "$REAL_TRACES/gcc.trace" 10 policy_foo
assert_stderr_contains "Replacement" "$REAL_TRACES/gcc.trace" 10 policy_foo

echo "PASS: test_invalid_policy"
