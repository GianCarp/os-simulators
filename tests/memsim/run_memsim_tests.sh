#!/bin/bash

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BIN="${MEMSIM:-$REPO_ROOT/build/memsim}"
UNIT_DIR="$REPO_ROOT/build/tests/memsim"
passed=0
failed=0

# Nothing here builds anything. Against a partly built tree every test fails
# describing its own symptom rather than the missing binary, so check the
# prerequisites once up front and run nothing if they are absent.
missing=0
[ -x "$BIN" ] || { echo "FAIL: $BIN not built"; missing=1; }
[ -d "$UNIT_DIR" ] || { echo "FAIL: $UNIT_DIR not built"; missing=1; }
if [ "$missing" -ne 0 ]; then
    echo "Run 'make test' to build memsim and its unit tests."
    exit 1
fi

# Integration tests
for test in "$SCRIPT_DIR"/integration/test_*.sh; do
    if bash "$test"; then
        passed=$((passed + 1))
    else
        failed=$((failed + 1))
    fi
done

# Unit tests. nullglob so an empty directory iterates zero times instead of
# once over the unmatched pattern, which would silently skip every binary and
# report the integration count alone as a clean pass.
shopt -s nullglob
for test_bin in "$UNIT_DIR"/test_*; do
    if "$test_bin"; then
        passed=$((passed + 1))
    else
        failed=$((failed + 1))
    fi
done
shopt -u nullglob

echo ""
echo "Memsim tests: $passed passed, $failed failed"
if [ "$failed" -ne 0 ]; then
    exit 1
fi
