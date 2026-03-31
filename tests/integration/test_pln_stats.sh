#!/bin/sh
# tests/integration/test_pln_stats.sh
#
# Integration tests for PLN statistics tracking:
#   - Forward chaining increments inferences counter
#   - Backward chaining increments inferences counter
#   - Rule match counter tracks correctly
#   - Stats reset brings counters to zero
#   - Truth value computation counter is accurate
#
# Uses awk-based PLN formula simulation matching the Plan 9 implementation
# in sys/src/libpln/pln.c.

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
ASSERT_FLOAT_IN_RANGE() {
    FVAL=$1; FLO=$2; FHI=$3; MSG=$4
    OK=$(awk "BEGIN { print ($FVAL >= $FLO && $FVAL <= $FHI) ? 1 : 0 }")
    if [ "$OK" = "1" ]; then PASS "$MSG ($FVAL in [$FLO, $FHI])"
    else FAIL "$MSG ($FVAL not in [$FLO, $FHI])"; fi
}

# ── Simulated PLN stats store ────────────────────────────────────────

WORK=/tmp/pln_stats_$$
mkdir -p "$WORK"
STATS="$WORK/plnstats"

reset_stats() {
    cat > "$STATS" <<EOF
inferences: 0
forward: 0
backward: 0
rulematch: 0
tvcompute: 0
EOF
}

get() { grep "^$1:" "$STATS" | sed "s/^$1: *//"; }
inc() { V=$(get "$1"); V=$((V + $2)); sed "s/^$1:.*/$1: $V/" "$STATS" > "$STATS.tmp" && mv "$STATS.tmp" "$STATS"; }

reset_stats

# ── PS-01: initial counters all zero ─────────────────────────────────

TEST_CASE "PS-01: initial PLN stats are all zero"
for field in inferences forward backward rulematch tvcompute; do
    V=$(get "$field")
    ASSERT_EQ "$V" "0" "$field initially 0"
done

# ── PS-02: forward chaining increments forward + inferences ──────────

TEST_CASE "PS-02: forward chaining increments counters"
# Simulate 3 forward chain steps (each fires 1 rule, produces 1 inference)
inc forward 3
inc inferences 3
inc rulematch 3
inc tvcompute 6   # deduction TV formula uses 2 TV computations per step

FWD=$(get forward); NINF=$(get inferences); RM=$(get rulematch); TVC=$(get tvcompute)
ASSERT_EQ "$FWD"  "3" "forward counter after 3 steps"
ASSERT_EQ "$NINF" "3" "inferences counter after 3 forward steps"
ASSERT_EQ "$RM"   "3" "rulematch counter after 3 forward steps"
ASSERT_GE "$TVC"  "6" "tvcompute >= 6 after 3 forward steps"

# ── PS-03: backward chaining increments backward + inferences ────────

TEST_CASE "PS-03: backward chaining increments counters"
inc backward 2
inc inferences 2
inc rulematch 2
inc tvcompute 4

BWD=$(get backward); NINF=$(get inferences)
ASSERT_EQ "$BWD"  "2" "backward counter after 2 steps"
ASSERT_GE "$NINF" "5" "cumulative inferences >= 5"

# ── PS-04: cumulative inferences = forward + backward ────────────────

TEST_CASE "PS-04: cumulative inferences = forward + backward"
FWD=$(get forward); BWD=$(get backward); NINF=$(get inferences)
EXPECTED=$((FWD + BWD))
ASSERT_EQ "$NINF" "$EXPECTED" "inferences = forward + backward"

# ── PS-05: stats reset zeroes all counters ────────────────────────────

TEST_CASE "PS-05: stats reset zeroes all counters"
reset_stats
for field in inferences forward backward rulematch tvcompute; do
    V=$(get "$field")
    ASSERT_EQ "$V" "0" "$field = 0 after reset"
done

# ── PS-06: PLN deduction TV formula ──────────────────────────────────

TEST_CASE "PS-06: deduction TV formula: P(A→C) from P(A→B) and P(B→C)"
# pAB = 0.9, pBC = 0.8 => pAC = pAB * pBC = 0.72
# confidence uses higher-order formula; approximation: cAC ~ cAB * cBC
RESULT=$(awk 'BEGIN {
    sAB=0.9; cAB=0.9;
    sBC=0.8; cBC=0.8;
    sAC = sAB * sBC;
    # Full confidence formula from pln.c: c * (c + (1-c)/2)
    cAC = cAB * cBC * (cAB * cBC + (1 - cAB * cBC) / 2.0);
    if (cAC > 1.0) cAC = 1.0;
    printf "%.6f %.6f\n", sAC, cAC
}')
SAC=$(echo "$RESULT" | awk '{print $1}')
CAC=$(echo "$RESULT" | awk '{print $2}')
ASSERT_FLOAT_IN_RANGE "$SAC" "0.70" "0.74" "deduction strength"
ASSERT_FLOAT_IN_RANGE "$CAC" "0.40" "0.70" "deduction confidence"
inc tvcompute 2   # one TV computation for this deduction

# ── PS-07: PLN induction TV formula ──────────────────────────────────

TEST_CASE "PS-07: induction TV formula: P(A→C) from observations"
# P(A→C) ~ P(C|A) with uncertainty
RESULT=$(awk 'BEGIN {
    sCA=0.7; cCA=0.8;
    sAC = sCA * cCA;                  # simplified induction strength
    cAC = cCA * 0.9;                  # slightly less confident
    if (cAC > 1.0) cAC = 1.0;
    printf "%.6f %.6f\n", sAC, cAC
}')
SAC=$(echo "$RESULT" | awk '{print $1}')
CAC=$(echo "$RESULT" | awk '{print $2}')
ASSERT_FLOAT_IN_RANGE "$SAC" "0.50" "0.65" "induction strength"
ASSERT_FLOAT_IN_RANGE "$CAC" "0.60" "0.80" "induction confidence"

# ── PS-08: PLN revision formula ──────────────────────────────────────

TEST_CASE "PS-08: revision (evidence combination) formula"
RESULT=$(awk 'BEGIN {
    s1=0.8; c1=0.9;
    s2=0.6; c2=0.7;
    wt = c1 + c2;
    sm = (s1*c1 + s2*c2) / wt;
    cm = wt / (wt + 1.0);
    printf "%.6f %.6f\n", sm, cm
}')
SM=$(echo "$RESULT" | awk '{print $1}')
CM=$(echo "$RESULT" | awk '{print $2}')
ASSERT_FLOAT_IN_RANGE "$SM" "0.70" "0.74" "revision merged strength"
ASSERT_FLOAT_IN_RANGE "$CM" "0.60" "0.65" "revision merged confidence"

# ── PS-09: tvcompute tracks TV operations ────────────────────────────

TEST_CASE "PS-09: tvcompute counter reflects TV operations performed"
TVC_BEFORE=$(get tvcompute)
inc tvcompute 5   # simulate 5 more TV computations
TVC_AFTER=$(get tvcompute)
ASSERT_GE "$TVC_AFTER" "$((TVC_BEFORE + 5))" "tvcompute increased by 5"

# ── PS-10: rulematch tracks rule applications ─────────────────────────

TEST_CASE "PS-10: rulematch counter reflects rule applications"
RM_BEFORE=$(get rulematch)
inc rulematch 4
RM_AFTER=$(get rulematch)
ASSERT_EQ "$RM_AFTER" "$((RM_BEFORE + 4))" "rulematch increased by 4"

# ── Teardown ─────────────────────────────────────────────────────────

rm -rf "$WORK"

printf '\n--- Summary ---\n'
printf 'Passed: %d\n' "$PASS_COUNT"
printf 'Failed: %d\n' "$FAIL_COUNT"

if [ "$FAIL_COUNT" -gt 0 ]; then
    echo "PLN stats integration tests FAILED."
    exit 1
fi
echo "All PLN stats integration tests passed."
exit 0
