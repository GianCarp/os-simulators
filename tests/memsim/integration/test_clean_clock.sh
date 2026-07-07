#!/bin/bash

# Clean-clock should prefer evicting clean pages over dirty ones, producing
# fewer disk writes than regular clock on the same trace.
#
# Setup: 3 frames, VPNs 1 and 2 are writes (dirty), VPN 3 is just a read (clean).
# First eviction: all ref bits are 1, pass 1 clears them, pass 2 evicts
#   VPN 1 (dirty). Both clock variants behave the same here. 1 disk write.
# Second eviction: VPN 2 (ref=0, dirty) and VPN 3 (ref=0, clean) are
#   candidates.
#   - clean-clock skips VPN 2 (dirty) and evicts VPN 3 (clean). 0 disk writes.
#   - regular clock evicts VPN 2 (first ref=0 it finds). 1 disk write.

set -e

source "$(dirname "$0")/../helpers.sh"
TRACE="$TRACES/clean_clock.trace"

# Both see 5 faults
assert_memsim_stat "total disk reads:" 5 "$TRACE" 3 clean-clock
assert_memsim_stat "total disk reads:" 5 "$TRACE" 3 clock

# Clean-clock produces fewer disk writes by preferring clean evictions
assert_memsim_stat "total disk writes:" 1 "$TRACE" 3 clean-clock
assert_memsim_stat "total disk writes:" 2 "$TRACE" 3 clock

echo "PASS: test_clean_clock"
