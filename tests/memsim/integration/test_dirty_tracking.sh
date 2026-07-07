#!/bin/bash

# Verifies that only dirty pages produce disk writes on eviction. With 2 frames
# under FIFO, VPN 1 is written (dirty) and VPN 2 is read (clean). When both
# are evicted, only VPN 1 should cause a disk write.

set -e

source "$(dirname "$0")/../helpers.sh"
TRACE="$TRACES/dirty_tracking.trace"

# One write from evicting the dirty page, zero from the clean page
assert_memsim_stat "total disk reads:" 4 "$TRACE" 2 fifo
assert_memsim_stat "total disk writes:" 1 "$TRACE" 2 fifo

echo "PASS: test_dirty_tracking"
