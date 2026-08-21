#!/bin/bash

# An unrecognised flag should exit with the unknown option code.

set -e

source "$(dirname "$0")/../helpers.sh"

assert_exit_code $EXIT_UNKNOWN_OPTION "$REAL_TRACES/gcc.trace" 10 clock -foo

echo "PASS: test_unknown_flag"
