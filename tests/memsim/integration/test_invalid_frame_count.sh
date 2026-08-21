#!/bin/bash

set -e

source "$(dirname "$0")/../helpers.sh"

# Both of these reach the same is_uint() rejection, so both report the same
# code. '-5' fails on the leading '-' exactly as 'foo' fails on the 'f';
# memsim has no separate notion of a negative frame count.
assert_exit_code $EXIT_FRAMES_FORMAT "$REAL_TRACES/gcc.trace" foo clock
assert_exit_code $EXIT_FRAMES_FORMAT "$REAL_TRACES/gcc.trace" -5 clock

# Zero passes is_uint() but fails the >= 1 check
assert_exit_code $EXIT_FRAMES_RANGE "$REAL_TRACES/gcc.trace" 0 clock

echo "PASS: test_invalid_frame_count"
