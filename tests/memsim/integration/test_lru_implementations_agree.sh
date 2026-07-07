#!/bin/bash

# lru-simple and lru-advanced must always produce identical fault rates.
# They are different implementations of the same policy.

set -e

source "$(dirname "$0")/../helpers.sh"

for trace in "$TRACES"/*.trace; do
    simple=$(echo "$("$MEMSIM" "$trace" 50 lru-simple 2>/dev/null)" | grep "total disk reads:" | awk '{print $NF}')
    advanced=$(echo "$("$MEMSIM" "$trace" 50 lru-advanced 2>/dev/null)" | grep "total disk reads:" | awk '{print $NF}')
    if [ "$simple" != "$advanced" ]; then
        echo "FAIL: lru-simple ($simple) != lru-advanced ($advanced) on $(basename "$trace")"
        exit 1
    fi
done

echo "PASS: test_lru_implementations_agree"
