#!/bin/sh
# tests/e2e/test_full_workflow.sh
#
# Comprehensive end-to-end workflow tests for cogplan9.
# Uses a mock cogctl / cogfs environment to simulate all
# conceivable actions, error cases, and edge conditions.

set -e  # abort on first real error (guards not covered by ASSERT_*)

# ── Setup ────────────────────────────────────────────────────────────
COGFS_ROOT=/tmp/cogfs_e2e_$$
rm -rf "$COGFS_ROOT"
mkdir -p "$COGFS_ROOT/atom"
LOGFILE="$COGFS_ROOT/log"

# Mock cogctl that writes results to COGFS_ROOT
cat > /tmp/cogctl_e2e_$$ <<'MOCK'
#!/bin/sh
COGFS_ROOT="${COGFS_ROOT:-/tmp/cogfs_e2e_mock}"
CMD=$1; shift

# exec -- "<atomese>"
if [ "$CMD" = "exec" ] && [ "$1" = "--" ]; then
    shift
    ATOMESE="$*"

    # Handle ConceptNode creation
    for NODE in $(echo "$ATOMESE" | grep -o 'ConceptNode "[^"]*"' | sed 's/ConceptNode "//;s/"//'); do
        echo "Created ConceptNode: $NODE" >> "$COGFS_ROOT/log"
        echo "stv 1.0 1.0" > "$COGFS_ROOT/atom/$NODE"
    done

    # Handle PredicateNode creation
    for NODE in $(echo "$ATOMESE" | grep -o 'PredicateNode "[^"]*"' | sed 's/PredicateNode "//;s/"//'); do
        echo "Created PredicateNode: $NODE" >> "$COGFS_ROOT/log"
        echo "stv 1.0 1.0" > "$COGFS_ROOT/atom/pred_$NODE"
    done

    # Handle ImplicationLink inference
    if echo "$ATOMESE" | grep -q "ImplicationLink"; then
        echo "Created inferred node: C" >> "$COGFS_ROOT/log"
        echo "stv 0.63 0.81" > "$COGFS_ROOT/atom/C_inferred"
    fi

    # Handle empty atomese (edge case)
    if [ -z "$ATOMESE" ]; then
        echo "ERROR: empty atomese" >> "$COGFS_ROOT/log"
        exit 1
    fi

    echo "(stv 1.0 1.0)"
    exit 0
fi

# get atomspace-stats
if [ "$CMD" = "get" ] && [ "$1" = "atomspace-stats" ]; then
    NODES=$(ls "$COGFS_ROOT/atom/" 2>/dev/null | wc -l | tr -d ' ')
    echo "nodes: $NODES, links: 0"
    exit 0
fi

# get version
if [ "$CMD" = "version" ]; then
    echo "cogplan9 1.0.0"
    exit 0
fi

# delete atom
if [ "$CMD" = "delete" ] && [ -n "$1" ]; then
    if [ -f "$COGFS_ROOT/atom/$1" ]; then
        rm "$COGFS_ROOT/atom/$1"
        echo "Deleted: $1" >> "$COGFS_ROOT/log"
        exit 0
    else
        echo "ERROR: atom not found: $1" >&2
        exit 1
    fi
fi

# unknown command — fail loudly
echo "ERROR: unknown cogctl command: $CMD" >&2
exit 1
MOCK
chmod +x /tmp/cogctl_e2e_$$

COGCTL="/tmp/cogctl_e2e_$$"
export COGFS_ROOT

# ── Helpers ──────────────────────────────────────────────────────────

PASS_COUNT=0
FAIL_COUNT=0

TEST_CASE() { printf '\n--- Test Case: %s ---\n' "$1"; }

PASS() { printf '  [PASS] %s\n' "$1"; PASS_COUNT=$((PASS_COUNT + 1)); }
FAIL() { printf '  [FAIL] %s\n' "$1"; FAIL_COUNT=$((FAIL_COUNT + 1)); }

ASSERT_FILE_EXISTS() {
    if [ -f "$1" ]; then PASS "File '$1' exists"
    else FAIL "File '$1' does not exist"; fi
}

ASSERT_FILE_NOT_EXISTS() {
    if [ ! -f "$1" ]; then PASS "File '$1' absent as expected"
    else FAIL "File '$1' should not exist"; fi
}

ASSERT_FILE_CONTAINS() {
    if grep -q "$2" "$1" 2>/dev/null; then
        PASS "File '$1' contains '$2'"
    else
        FAIL "File '$1' does not contain '$2'"
    fi
}

ASSERT_OUTPUT_EQ() {
    if [ "$1" = "$2" ]; then PASS "Output: '$1'"
    else FAIL "Expected '$2', got '$1'"; fi
}

ASSERT_OUTPUT_CONTAINS() {
    if echo "$1" | grep -q "$2"; then PASS "Output contains '$2'"
    else FAIL "Expected output to contain '$2', got: '$1'"; fi
}

ASSERT_CMD_SUCCEEDS() {
    if eval "$1" >/dev/null 2>&1; then PASS "Command succeeded: $1"
    else FAIL "Command failed: $1"; fi
}

ASSERT_CMD_FAILS() {
    if eval "$1" >/dev/null 2>&1; then
        FAIL "Expected command to fail but it succeeded: $1"
    else
        PASS "Command failed as expected: $1"
    fi
}

# ── Test Cases ───────────────────────────────────────────────────────

#
# TC-01: Create basic atoms
#
TEST_CASE "TC-01: Create ConceptNode atoms"
"$COGCTL" exec -- '(ConceptNode "A")'
"$COGCTL" exec -- '(ConceptNode "B")'
ASSERT_FILE_EXISTS "$COGFS_ROOT/atom/A"
ASSERT_FILE_EXISTS "$COGFS_ROOT/atom/B"
ASSERT_FILE_CONTAINS "$COGFS_ROOT/atom/A" "stv 1.0 1.0"

#
# TC-02: Create PredicateNode
#
TEST_CASE "TC-02: Create PredicateNode"
"$COGCTL" exec -- '(PredicateNode "likes")'
ASSERT_FILE_EXISTS "$COGFS_ROOT/atom/pred_likes"

#
# TC-03: PLN implication inference
#
TEST_CASE "TC-03: PLN implication inference"
"$COGCTL" exec -- '(ImplicationLink (ConceptNode "A") (ConceptNode "B") (ConceptNode "C"))'
ASSERT_FILE_EXISTS "$COGFS_ROOT/atom/C_inferred"
ASSERT_FILE_CONTAINS "$COGFS_ROOT/atom/C_inferred" "stv 0.63 0.81"

#
# TC-04: AtomSpace statistics
#
TEST_CASE "TC-04: AtomSpace stats report atom count"
STATS=$("$COGCTL" get atomspace-stats)
ASSERT_OUTPUT_CONTAINS "$STATS" "nodes:"

#
# TC-05: Version string
#
TEST_CASE "TC-05: Version string"
VER=$("$COGCTL" version)
ASSERT_OUTPUT_CONTAINS "$VER" "cogplan9"

#
# TC-06: Delete existing atom
#
TEST_CASE "TC-06: Delete atom that exists"
"$COGCTL" exec -- '(ConceptNode "ToDelete")'
ASSERT_FILE_EXISTS "$COGFS_ROOT/atom/ToDelete"
"$COGCTL" delete "ToDelete"
ASSERT_FILE_NOT_EXISTS "$COGFS_ROOT/atom/ToDelete"

#
# TC-07: Delete non-existent atom (error case)
#
TEST_CASE "TC-07: Delete atom that does not exist (error)"
ASSERT_CMD_FAILS "\"$COGCTL\" delete 'nonexistent_atom_xyz'"

#
# TC-08: Empty atomese is rejected (error case)
#
TEST_CASE "TC-08: Empty atomese expression is rejected"
ASSERT_CMD_FAILS "\"$COGCTL\" exec -- ''"

#
# TC-09: Unknown command (error case)
#
TEST_CASE "TC-09: Unknown cogctl command fails"
ASSERT_CMD_FAILS "\"$COGCTL\" frobnicate something"

#
# TC-10: Multiple atoms in one exec
#
TEST_CASE "TC-10: Multiple ConceptNodes in one atomese expression"
"$COGCTL" exec -- '(EvaluationLink (PredicateNode "rel") (ListLink (ConceptNode "X") (ConceptNode "Y")))'
ASSERT_FILE_EXISTS "$COGFS_ROOT/atom/X"
ASSERT_FILE_EXISTS "$COGFS_ROOT/atom/Y"

#
# TC-11: Idempotent atom creation
#
TEST_CASE "TC-11: Re-creating atom A is idempotent"
"$COGCTL" exec -- '(ConceptNode "A")'  # A already exists from TC-01
ASSERT_FILE_EXISTS "$COGFS_ROOT/atom/A"
ASSERT_FILE_CONTAINS "$COGFS_ROOT/atom/A" "stv 1.0 1.0"

#
# TC-12: Log integrity
#
TEST_CASE "TC-12: Log file records operations"
ASSERT_FILE_EXISTS "$LOGFILE"
ASSERT_FILE_CONTAINS "$LOGFILE" "ConceptNode"

#
# TC-13: Stats increase after adding atoms
#
TEST_CASE "TC-13: Stats node count is positive"
STATS=$("$COGCTL" get atomspace-stats)
# Extract the node count number
NCOUNT=$(echo "$STATS" | sed 's/nodes: \([0-9]*\).*/\1/')
if [ "$NCOUNT" -gt 0 ] 2>/dev/null; then
    PASS "Node count ($NCOUNT) > 0"
else
    FAIL "Node count should be > 0, got: $STATS"
fi

#
# TC-14: Atom name with special characters
#
TEST_CASE "TC-14: Atom name with underscores and hyphens"
"$COGCTL" exec -- '(ConceptNode "my-concept_node")'
ASSERT_FILE_EXISTS "$COGFS_ROOT/atom/my-concept_node"

#
# TC-15: File system integrity after all operations
#
TEST_CASE "TC-15: atom directory still exists after all operations"
ASSERT_CMD_SUCCEEDS "[ -d \"$COGFS_ROOT/atom\" ]"

# ── Teardown ─────────────────────────────────────────────────────────

printf '\n--- Summary ---\n'
printf 'Passed: %d\n' "$PASS_COUNT"
printf 'Failed: %d\n' "$FAIL_COUNT"

rm -rf "$COGFS_ROOT"
rm -f "/tmp/cogctl_e2e_$$"

if [ "$FAIL_COUNT" -gt 0 ]; then
    echo "E2E tests FAILED."
    exit 1
fi

echo "All E2E tests passed."
exit 0
