#!/bin/bash

# Invalid uses of the -seed flag should exit with the code for the specific
# failure. A missing value and a malformed value are separate code paths.

set -e

source "$(dirname "$0")/../helpers.sh"

# -seed with no value following it
assert_exit_code $EXIT_SEED_MISSING "$REAL_TRACES/gcc.trace" 10 clock -seed

# -seed with a non-numeric value
assert_exit_code $EXIT_SEED_FORMAT "$REAL_TRACES/gcc.trace" 10 clock -seed abc

echo "PASS: test_seed_errors"
