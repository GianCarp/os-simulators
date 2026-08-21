#!/bin/bash

# The -h flag should print usage and exit successfully.

set -e

source "$(dirname "$0")/../helpers.sh"

assert_exit_code $EXIT_OK -h

echo "PASS: test_help_flag"
