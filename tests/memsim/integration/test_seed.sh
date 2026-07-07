#!/bin/bash

# Invalid uses of the -seed flag should print the appropriate error message.
set -e

source "$(dirname "$0")/../helpers.sh"

# -seed with no value following it
assert_exit_code 1 "$REAL_TRACES/gcc.trace" 10 clock -seed
assert_stderr_contains "requires" "$REAL_TRACES/gcc.trace" 10 clock -seed

# -seed with a non-numeric value
assert_exit_code 1 "$REAL_TRACES/gcc.trace" 10 clock -seed abc
assert_stderr_contains "Invalid" "$REAL_TRACES/gcc.trace" 10 clock -seed abc

echo "PASS: test_seed_errors"
