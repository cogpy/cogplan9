#!/bin/sh
# tests/integration/run_integration_tests.sh
#
# Runner for all Phase 2E integration tests:
#   - coginfo statistics accuracy
#   - AtomSpace export/import persistence roundtrip
#   - PLN statistics tracking
#   - Performance benchmarks

set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/../.." && pwd)

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

PASS=0
FAIL=0

run_test() {
    NAME=$1
    SCRIPT=$2
    LOGFILE="/tmp/integ_test_$$.log"
    printf "${YELLOW}[RUN]${NC} %s\n" "$NAME"
    if sh "$SCRIPT" > "$LOGFILE" 2>&1; then
        printf "${GREEN}[PASS]${NC} %s\n\n" "$NAME"
        PASS=$((PASS + 1))
    else
        printf "${RED}[FAIL]${NC} %s\n" "$NAME"
        cat "$LOGFILE"
        printf "\n"
        FAIL=$((FAIL + 1))
    fi
    rm -f "$LOGFILE"
}

echo "=============================================="
echo "Plan9Cog Phase 2E Integration Test Suite"
echo "=============================================="
echo ""

run_test "coginfo statistics"          "$SCRIPT_DIR/test_coginfo_stats.sh"
run_test "AtomSpace persistence"       "$SCRIPT_DIR/test_atomspace_persistence.sh"
run_test "PLN statistics tracking"     "$SCRIPT_DIR/test_pln_stats.sh"
run_test "Performance benchmarks"      "$SCRIPT_DIR/test_benchmark.sh"

echo "=============================================="
echo "Integration Test Summary"
echo "=============================================="
printf "${GREEN}Passed: %d${NC}\n" "$PASS"
if [ "$FAIL" -gt 0 ]; then
    printf "${RED}Failed: %d${NC}\n" "$FAIL"
    echo "INTEGRATION TESTS FAILED."
    exit 1
fi
printf "Failed: %d\n" "$FAIL"
echo ""
printf "${GREEN}ALL INTEGRATION TESTS PASSED${NC}\n"
exit 0
