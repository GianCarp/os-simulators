#!/bin/bash

# Accessing 5 distinct VPNs with 5 frames means every access is a page fault
# but no eviction ever occurs as there is always a free frame available.
# Disk writes should be 0 since no pages are evicted.

set -e

source "$(dirname "$0")/../helpers.sh"
TRACE="$TRACES/five_unique_vpns.trace"

for policy in lru-simple lru-advanced fifo rand clock clean-clock; do
    assert_memsim_stat "total disk reads:" 5 "$TRACE" 5 "$policy"
    assert_memsim_stat "total disk writes:" 0 "$TRACE" 5 "$policy"
done

echo "PASS: test_all_access_page_faults_no_eviction"
