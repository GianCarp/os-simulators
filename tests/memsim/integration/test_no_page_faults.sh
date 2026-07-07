#!/bin/bash

# Repeatedly accessing the same VPN should produce exactly one page fault
# (the initial compulsory miss) and zero disk writes, regardless of policy
# or frame count.
#
# Note: test name implies no page faults besides the compulsory miss.

set -e

source "$(dirname "$0")/../helpers.sh"
TRACE="$TRACES/all_hits.trace"

for policy in lru-simple lru-advanced fifo rand clock clean-clock; do
    assert_memsim_stat "total disk reads:" 1 "$TRACE" 1 "$policy"
    assert_memsim_stat "total disk writes:" 0 "$TRACE" 1 "$policy"
done

echo "PASS: test_no_page_faults"
