#!/bin/bash

# Running memsim with no args should exit with the usage code.

set -e

source "$(dirname "$0")/../helpers.sh"

assert_exit_code $EXIT_USAGE

echo "PASS: test_no_args"
