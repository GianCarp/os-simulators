# Shared helpers for memsim integration tests.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MEMSIM="${MEMSIM:-$SCRIPT_DIR/../../build/memsim}"
TRACES="$SCRIPT_DIR/integration/traces"
REAL_TRACES="$SCRIPT_DIR/../../memsim/traces"

# Check that memsim exits with a specific exit code. Takes the expected exit
# code as the first argument, and then any remaining arguments are passed to
# memsim. On failure, dumps the expected vs actual exit code, the arguments
# used, and the full output so debugging does not require re-running.
#
# Usage: assert_exit_code <expected_code> <args...>

assert_exit_code() {
    local expected=$1
    shift
    local output
    local actual
    set +e
    output=$("$MEMSIM" "$@" 2>&1)
    actual=$?
    set -e
    if [ "$actual" -ne "$expected" ]; then
        echo "FAIL: expected exit $expected, got $actual"
        echo "  args: $*"
        echo "  output: $output"
        exit 1
    fi
}

# Check that memsim's stderr output contains a specific substring. Takes the
# expected substring as the first argument, and then any remaining arguments
# are passed to memsim. Only stderr is captured -- stdout is discarded, so the
# check is not polluted by normal output like the stats table.
#
# Usage: assert_stderr_contains <substring> <args...>

assert_stderr_contains() {
    local needle=$1
    shift
    local output
    set +e
    output=$("$MEMSIM" "$@" 2>&1 >/dev/null)
    set -e
    if ! echo "$output" | grep -q "$needle"; then
        echo "FAIL: expected stderr to contain '$needle'"
        echo "  args: $*"
        echo "  stderr: $output"
        exit 1
    fi
}

# Check that a specific statistic in memsim's output matches an expected value.
# Takes the stat name (e.g. "total disk reads:") as the first argument, the
# expected value as the second, and then any remaining arguments are passed to
# memsim. The stat value is extracted by finding the line matching the stat name
# and taking the last whitespace-separated field on that line.
#
# Usage: assert_memsim_stat <stat_name> <expected_value> <args...>

assert_memsim_stat() {
    local stat_name=$1
    local expected=$2
    shift 2
    local output
    set +e
    output=$("$MEMSIM" "$@" 2>/dev/null)
    set -e
    local actual
    actual=$(echo "$output" | grep "$stat_name" | awk '{print $NF}')
    if [ "$actual" != "$expected" ]; then
        echo "FAIL: $stat_name expected $expected, got $actual"
        echo "  args: $*"
        echo "  output:"
        echo "$output"
        exit 1
    fi
}
