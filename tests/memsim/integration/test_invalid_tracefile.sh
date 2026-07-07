#!/bin/bash

# A non-existent tracefile should print the appropriate error message.

set -e

source "$(dirname "$0")/../helpers.sh"

assert_exit_code 1 /no/such/file.trace 50 clock
assert_stderr_contains "Cannot" /no/such/file.trace 50 clock

echo "PASS: test_invalid_tracefile"
