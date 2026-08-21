#!/bin/bash

# A non-existent tracefile should exit with the trace open code.

set -e

source "$(dirname "$0")/../helpers.sh"

assert_exit_code $EXIT_TRACE_OPEN /no/such/file.trace 50 clock

echo "PASS: test_invalid_tracefile"
