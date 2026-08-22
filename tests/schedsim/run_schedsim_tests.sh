#!/bin/bash

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
passed=0
failed=0

# Integration tests
for test in "$SCRIPT_DIR"/integration/test_*.sh; do
    if bash "$test"; then
        passed=$((passed + 1))
    else
        failed=$((failed + 1))
    fi
done

# Unit tests
for test_bin in "$SCRIPT_DIR"/../../build/tests/schedsim/test_*; do
    if [ -x "$test_bin" ]; then
        if "$test_bin"; then
            passed=$((passed + 1))
        else
            failed=$((failed + 1))
        fi
    fi
done

echo ""
echo "Schedsim tests: $passed passed, $failed failed"
if [ "$failed" -ne 0 ]; then
    exit 1
fi
