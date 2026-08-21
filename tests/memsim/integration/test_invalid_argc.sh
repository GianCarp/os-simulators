#!/bin/bash

# Passing fewer than 3 or more than 6 arguments (excluding the binary name)
# should exit with the usage code.

set -e

source "$(dirname "$0")/../helpers.sh"

# Too few arguments, noting the case of 0 args is handled by test_no_args.sh
assert_exit_code $EXIT_USAGE a
assert_exit_code $EXIT_USAGE a b

# Too many arguments
assert_exit_code $EXIT_USAGE a b c d e f g

echo "PASS: test_invalid_argc"
