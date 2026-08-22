#!/bin/bash

# -h prints usage and exits successfully, without needing a workload file.

set -e

source "$(dirname "$0")/../helpers.sh"

assert_exit_code $EXIT_OK -h

echo "PASS: test_help_flag"
