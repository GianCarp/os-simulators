#!/bin/bash

# A malformed tracefile must be rejected rather than silently truncating the
# run. Every line has to match "<hex address> <R|W>". A line that does not is
# an error regardless of how it fails to match, so all four traces below report
# the same code. Which line and why is left to the stderr message.
#
# The truncated and bad-address cases matter most: before line-oriented
# parsing, both ended the read loop as though the file had simply finished, so
# memsim exited 0 and printed a stats table computed from a fraction of the
# trace.

set -e

source "$(dirname "$0")/../helpers.sh"

# Operation character present but not R or W
assert_exit_code $EXIT_TRACE_FORMAT "$TRACES/malformed_bad_op.trace" 4 clock

# Operation character missing, with valid lines following it
assert_exit_code $EXIT_TRACE_FORMAT "$TRACES/malformed_missing_op.trace" 4 clock

# Operation character missing on the final line
assert_exit_code $EXIT_TRACE_FORMAT "$TRACES/malformed_truncated.trace" 4 clock

# Address is not hexadecimal
assert_exit_code $EXIT_TRACE_FORMAT "$TRACES/malformed_bad_address.trace" 4 clock

echo "PASS: test_trace_format"
