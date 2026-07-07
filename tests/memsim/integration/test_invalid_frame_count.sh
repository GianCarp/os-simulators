#!/bin/bash

# An invalid entry for number of frames should print the appropriate error message.

set -e

source "$(dirname "$0")/../helpers.sh"

# Non-numeric input fails is_uint(). -5 is caught here too as the leading
# '-' is not a digit, so is_uint() rejects.

assert_exit_code 1 "$REAL_TRACES/gcc.trace" foo clock
assert_exit_code 1 "$REAL_TRACES/gcc.trace" -5 clock
assert_stderr_contains "Invalid" "$REAL_TRACES/gcc.trace" foo clock
assert_stderr_contains "Invalid" "$REAL_TRACES/gcc.trace" -5 clock

# Zero passes is_uint() but fails the >= 1 check
assert_exit_code 1 "$REAL_TRACES/gcc.trace" 0 clock
assert_stderr_contains "at least 1" "$REAL_TRACES/gcc.trace" 0 clock

echo "PASS: test_invalid_frame_count"
