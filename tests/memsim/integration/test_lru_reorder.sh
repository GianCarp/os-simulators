#!/bin/bash

# Tests that LRU's recency tracking produces different eviction
# decisions than FIFO. The access pattern is VPN 1, 2, 3, 1, 4, 1 with 3
# frames. After loading VPNs 1-3, VPN 1 is accessed again making it the
# most recently used. When VPN 4 arrives:
#   - LRU evicts VPN 2 (least recently used), so the final access to VPN 1
#     is a hit. 4 faults total.
#   - FIFO evicts VPN 1 (first loaded), so the final access to VPN 1 is a
#     fault. 5 faults total.
# Both LRU implementations must agree.

set -e

source "$(dirname "$0")/../helpers.sh"
TRACE="$TRACES/lru_reorder.trace"

# LRU keeps VPN 1 because it was recently accessed
assert_memsim_stat "total disk reads:" 4 "$TRACE" 3 lru-simple
assert_memsim_stat "total disk reads:" 4 "$TRACE" 3 lru-advanced

assert_memsim_stat "total disk writes:" 0 "$TRACE" 3 lru-simple
assert_memsim_stat "total disk writes:" 0 "$TRACE" 3 lru-advanced

# FIFO evicts VPN 1 because it was loaded first
assert_memsim_stat "total disk reads:" 5 "$TRACE" 3 fifo
assert_memsim_stat "total disk writes:" 0 "$TRACE" 3 fifo

echo "PASS: test_lru_reorder"
