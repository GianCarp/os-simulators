#!/bin/bash

# An unrecognised flag should print the appropriate error message.

set -e

source "$(dirname "$0")/../helpers.sh"

assert_exit_code 1 "$REAL_TRACES/gcc.trace" 10 clock -banana
assert_stderr_contains "Unknown" "$REAL_TRACES/gcc.trace" 10 clock -foo

echo "PASS: test_unknown_flag"
