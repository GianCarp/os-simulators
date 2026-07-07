#!/bin/bash

# Accessing 5 distinct VPNs with only 1 frame means every access is a page
# fault and every access after the first triggers an eviction. No writes occur
# because all accesses are reads, so evicted pages are always clean.

set -e

source "$(dirname "$0")/../helpers.sh"
TRACE="$TRACES/five_unique_vpns.trace"

for policy in lru-simple lru-advanced fifo rand clock clean-clock; do
    assert_memsim_stat "total disk reads:" 5 "$TRACE" 1 "$policy"
    assert_memsim_stat "total disk writes:" 0 "$TRACE" 1 "$policy"
done

echo "PASS: test_all_access_page_faults_with_eviction"
