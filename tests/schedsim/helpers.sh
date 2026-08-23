# Shared helpers for schedsim integration tests.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCHEDSIM="${SCHEDSIM:-$SCRIPT_DIR/../../build/schedsim}"
WORKLOADS="$SCRIPT_DIR/integration/workloads"
REAL_WORKLOADS="$SCRIPT_DIR/../../schedsim/workloads"

# Exit codes reported by schedsim, one per failure mode. Keep in sync with
# enum exit_code in schedsim/include/schedsim.h. 1 is absent deliberately, it
# is reserved for unexpected internal failure, so a test can never expect it.

readonly EXIT_OK=0
readonly EXIT_USAGE=2
readonly EXIT_BAD_POLICY=3
readonly EXIT_TIME_SLICE=4
readonly EXIT_WORKLOAD_OPEN=5
readonly EXIT_WORKLOAD_FORMAT=6
readonly EXIT_WORKLOAD_EMPTY=7
readonly EXIT_NO_MEMORY=8

# Check that schedsim exits with a specific exit code. Takes the expected exit
# code as the first argument, and then any remaining arguments are passed to
# schedsim. On failure, dumps the expected vs actual exit code, the arguments
# used, and the full output so debugging does not require re-running.
#
# Usage: assert_exit_code <expected_code> <args...>

assert_exit_code() {
    local expected=$1
    shift
    local output
    local actual
    set +e
    output=$("$SCHEDSIM" "$@" 2>&1)
    actual=$?
    set -e
    if [ "$actual" -ne "$expected" ]; then
        echo "FAIL: expected exit $expected, got $actual"
        echo "  args: $*"
        echo "  output: $output"
        exit 1
    fi
}

# Run schedsim once and cache its output. A single run reports every policy
# asked for, so the assertions that follow read the cached text rather than
# re-invoking the simulator per statistic.
#
# Usage: run_schedsim <args...>

run_schedsim() {
    SCHEDSIM_ARGS="$*"
    set +e
    SCHEDSIM_OUTPUT=$("$SCHEDSIM" "$@" 2>/dev/null)
    set -e
}

# Check a statistic from the most recent run_schedsim. Unlike memsim, schedsim
# prints a separate block per policy, so the policy name selects which block to
# read before the stat name is matched. Blocks are delimited by
# "===== Policy: <name> =====" banners.
#
# Usage: assert_stat <policy> <stat_name> <expected_value>

assert_stat() {
    local policy=$1
    local stat_name=$2
    local expected=$3

    # Narrow to the requested policy's block first, then get the stat. The
    # label is anchored so a job table row can never match it.
    local actual
    actual=$(echo "$SCHEDSIM_OUTPUT" | awk -v banner="===== Policy: $policy =====" '
        $0 == banner { in_block = 1; next }
        /^===== Policy: / { in_block = 0 }
        in_block' | grep "^  $stat_name" | awk '{print $NF}')

    if [ "$actual" != "$expected" ]; then
        echo "FAIL: [$policy] $stat_name expected $expected, got $actual"
        echo "  args: $SCHEDSIM_ARGS"
        echo "  output:"
        echo "$SCHEDSIM_OUTPUT"
        exit 1
    fi
}
