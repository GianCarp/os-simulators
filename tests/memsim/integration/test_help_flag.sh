#!/bin/bash

# The -h flag should print usage and exit successfully.

set -e

source "$(dirname "$0")/../helpers.sh"

assert_exit_code 0 -h
assert_stderr_contains "Usage" -h

echo "PASS: test_help_flag"
