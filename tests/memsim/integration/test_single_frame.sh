#!/bin/bash

# With a single frame, every access to a new VPN is a fault but consecutive
# accesses to the same VPN are hits. This is an edge case for the DLL where
# head and tail are always the same node, and lru_move_existing_node_head
# must handle the early return correctly.

set -e

source "$(dirname "$0")/../helpers.sh"
TRACE="$TRACES/single_frame.trace"

for policy in lru-simple lru-advanced fifo rand clock clean-clock; do
    assert_memsim_stat "total disk reads:" 4 "$TRACE" 1 "$policy"
    assert_memsim_stat "total disk writes:" 1 "$TRACE" 1 "$policy"
done

echo "PASS: test_single_frame"
