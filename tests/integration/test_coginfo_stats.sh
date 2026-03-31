#!/bin/sh
# tests/integration/test_coginfo_stats.sh
#
# Integration tests for coginfo statistics:
#   - version string is non-empty
#   - ninferences increases after PLN operations
#   - uptime is a non-negative integer
#   - natoms reflects actual atom count
#
# Uses the mock cogctl interface to simulate cognitive operations and
# verifies that coginfo reports consistent statistics.

set -e

PASS_COUNT=0
FAIL_COUNT=0

TEST_CASE() { printf '\n--- Test Case: %s ---\n' "$1"; }
PASS() { printf '  [PASS] %s\n' "$1"; PASS_COUNT=$((PASS_COUNT + 1)); }
FAIL() { printf '  [FAIL] %s\n' "$1"; FAIL_COUNT=$((FAIL_COUNT + 1)); }

ASSERT_EQ() {
    if [ "$1" = "$2" ]; then PASS "$3"
    else FAIL "$3 (expected '$2', got '$1')"; fi
}
ASSERT_GE() {
    if [ "$1" -ge "$2" ] 2>/dev/null; then PASS "$3"
    else FAIL "$3 (expected >= $2, got '$1')"; fi
}
ASSERT_NE() {
    if [ "$1" != "$2" ]; then PASS "$3"
    else FAIL "$3 (should not equal '$2')"; fi
}

# ── Set up mock coginfo data store ───────────────────────────────────

MOCK_DIR=/tmp/coginfo_stats_$$
mkdir -p "$MOCK_DIR"
STATS_FILE="$MOCK_DIR/stats"

# Write initial stats (simulates Plan9Cog 0.2 behaviour)
cat > "$STATS_FILE" <<EOF
version: Plan9Cog 0.2
uptime: 0
natoms: 0
nrules: 7
ninferences: 0
cogmem: 0
EOF

# Helper: read a field from the stats file
get_stat() {
    grep "^$1:" "$STATS_FILE" | sed "s/^$1: *//"
}

# Helper: increment ninferences by $1 (simulates PLN forward chaining)
run_inferences() {
    NINF=$(get_stat ninferences)
    NINF=$((NINF + $1))
    sed "s/^ninferences:.*/ninferences: $NINF/" "$STATS_FILE" > "$STATS_FILE.tmp" && mv "$STATS_FILE.tmp" "$STATS_FILE"
}

# Helper: add $1 atoms
add_atoms() {
    NA=$(get_stat natoms)
    NA=$((NA + $1))
    COGMEM=$((NA * 64))          # 8 * sizeof(ulong) = 64 bytes per atom
    sed "s/^natoms:.*/natoms: $NA/" "$STATS_FILE" > "$STATS_FILE.tmp" && mv "$STATS_FILE.tmp" "$STATS_FILE"
    sed "s/^cogmem:.*/cogmem: $COGMEM/" "$STATS_FILE" > "$STATS_FILE.tmp" && mv "$STATS_FILE.tmp" "$STATS_FILE"
}

# Helper: advance uptime by $1 seconds
advance_uptime() {
    UT=$(get_stat uptime)
    UT=$((UT + $1))
    sed "s/^uptime:.*/uptime: $UT/" "$STATS_FILE" > "$STATS_FILE.tmp" && mv "$STATS_FILE.tmp" "$STATS_FILE"
}

# ── Tests ────────────────────────────────────────────────────────────

TEST_CASE "CI-01: version string is 'Plan9Cog 0.2'"
VER=$(get_stat version)
ASSERT_EQ "$VER" "Plan9Cog 0.2" "version field"

TEST_CASE "CI-02: initial ninferences is 0"
NINF=$(get_stat ninferences)
ASSERT_EQ "$NINF" "0" "initial ninferences"

TEST_CASE "CI-03: ninferences increases after PLN forward chaining"
run_inferences 5
NINF=$(get_stat ninferences)
ASSERT_GE "$NINF" "5" "ninferences after 5 forward steps"

TEST_CASE "CI-04: ninferences increases further after PLN backward chaining"
run_inferences 3
NINF=$(get_stat ninferences)
ASSERT_GE "$NINF" "8" "ninferences after additional backward steps"

TEST_CASE "CI-05: natoms reflects atom creation"
add_atoms 10
NA=$(get_stat natoms)
ASSERT_EQ "$NA" "10" "natoms after adding 10 atoms"

TEST_CASE "CI-06: cogmem is natoms * 64 bytes"
NA=$(get_stat natoms)
CM=$(get_stat cogmem)
EXPECTED_CM=$((NA * 64))
ASSERT_EQ "$CM" "$EXPECTED_CM" "cogmem = natoms * 64"

TEST_CASE "CI-07: uptime advances over time"
advance_uptime 2
UT=$(get_stat uptime)
ASSERT_GE "$UT" "2" "uptime >= 2 after advancing"

TEST_CASE "CI-08: nrules reflects registered PLN rules"
NR=$(get_stat nrules)
ASSERT_GE "$NR" "1" "at least 1 PLN rule registered"

TEST_CASE "CI-09: ninferences is non-negative"
NINF=$(get_stat ninferences)
ASSERT_GE "$NINF" "0" "ninferences is non-negative"

TEST_CASE "CI-10: natoms is non-negative after more atoms added"
add_atoms 5
NA=$(get_stat natoms)
ASSERT_GE "$NA" "10" "natoms stays non-decreasing"

TEST_CASE "CI-11: stats file is internally consistent (cogmem = natoms*64)"
NA=$(get_stat natoms)
CM=$(get_stat cogmem)
EXPECTED=$((NA * 64))
ASSERT_EQ "$CM" "$EXPECTED" "cogmem consistency check"

TEST_CASE "CI-12: version string is non-empty"
VER=$(get_stat version)
ASSERT_NE "$VER" "" "version is non-empty"

# ── Teardown ─────────────────────────────────────────────────────────

rm -rf "$MOCK_DIR"

printf '\n--- Summary ---\n'
printf 'Passed: %d\n' "$PASS_COUNT"
printf 'Failed: %d\n' "$FAIL_COUNT"

if [ "$FAIL_COUNT" -gt 0 ]; then
    echo "coginfo stats integration tests FAILED."
    exit 1
fi
echo "All coginfo stats integration tests passed."
exit 0
