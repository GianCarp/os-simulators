#!/bin/bash

# The random policy with the same seed must produce identical results across
# multiple runs. This verifies that seeding works correctly and results are
# reproducible.

set -e

source "$(dirname "$0")/../helpers.sh"
TRACE="$TRACES/five_unique_vpns.trace"

first_run=$("$MEMSIM" "$TRACE" 3 rand -seed 5 2>/dev/null)
second_run=$("$MEMSIM" "$TRACE" 3 rand -seed 5 2>/dev/null)

if [ "$first_run" != "$second_run" ]; then
    echo "FAIL: random policy with same seed produced different results"
    echo "  first:  $first_run"
    echo "  second: $second_run"
    exit 1
fi

# Different seeds should produce different results
third_run=$("$MEMSIM" "$TRACE" 3 rand -seed 7 2>/dev/null)

if [ "$first_run" = "$third_run" ]; then
    echo "FAIL: different seeds produced identical results"
    exit 1
fi

echo "PASS: test_deterministic_random"
