#!/bin/bash

# Running memsim with no argus should print usage and exit with failure.

set -e

source "$(dirname "$0")/../helpers.sh"

assert_exit_code 1
assert_stderr_contains "Usage"

echo "PASS: test_no_args"
