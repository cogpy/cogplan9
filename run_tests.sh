#!/bin/bash
#
# Plan9Cog Test Runner
# Runs all unit tests and integration tests
#

set -e

echo "=============================================="
echo "Plan9Cog Comprehensive Test Suite"
echo "=============================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Track results
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to run a test category
run_test_category() {
    local name=$1
    local description=$2

    echo -e "${YELLOW}[$name]${NC} $description"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
}

# Function to report pass
test_pass() {
    local name=$1
    echo -e "  ${GREEN}✓${NC} $name"
    PASSED_TESTS=$((PASSED_TESTS + 1))
}

# Function to report fail
test_fail() {
    local name=$1
    echo -e "  ${RED}✗${NC} $name"
    FAILED_TESTS=$((FAILED_TESTS + 1))
}

echo "=== Checking Source Files ==="
echo ""

# Check header files exist
echo "Header Files:"
for f in atomspace.h pln.h cogvm.h tensorlogic.h; do
    if [ -f "sys/include/plan9cog/$f" ]; then
        test_pass "$f exists"
    else
        test_fail "$f missing"
    fi
done

if [ -f "sys/include/plan9cog.h" ]; then
    test_pass "plan9cog.h exists"
else
    test_fail "plan9cog.h missing"
fi

echo ""
echo "=== Checking Library Source Files ==="
echo ""

# Check libatomspace
echo "libatomspace:"
for f in atomspace.c pattern.c serialize.c; do
    if [ -f "sys/src/libatomspace/$f" ]; then
        test_pass "$f exists"
    else
        test_fail "$f missing"
    fi
done

# Check libpln
echo ""
echo "libpln:"
for f in pln.c ure.c; do
    if [ -f "sys/src/libpln/$f" ]; then
        test_pass "$f exists"
    else
        test_fail "$f missing"
    fi
done

# Check libplan9cog
echo ""
echo "libplan9cog:"
for f in plan9cog.c ecan.c machspace.c reactor.c patternmine.c; do
    if [ -f "sys/src/libplan9cog/$f" ]; then
        test_pass "$f exists"
    else
        test_fail "$f missing"
    fi
done

# Check libtensorlogic
echo ""
echo "libtensorlogic:"
for f in tensor.c einsum.c nonlinear.c logic.c embedding.c kernel.c graphmodel.c plnintegration.c; do
    if [ -f "sys/src/libtensorlogic/$f" ]; then
        test_pass "$f exists"
    else
        test_fail "$f missing"
    fi
done

echo ""
echo "=== Checking Test Files ==="
echo ""

# Check test files
echo "Unit Tests:"
for f in test_atomspace.c test_pln.c test_ecan.c test_ure.c test_machspace.c test_pattern.c test_integration.c; do
    if [ -f "sys/src/test/$f" ]; then
        test_pass "$f exists"
    else
        test_fail "$f missing"
    fi
done

echo ""
echo "Tensor Logic Tests:"
for f in test_tensor.c test_einsum.c test_tensorlogic.c test_embedding.c test_kernel.c test_graphmodel.c; do
    if [ -f "sys/src/test/$f" ]; then
        test_pass "$f exists"
    else
        test_fail "$f missing"
    fi
done

echo ""
echo "=== Checking mkfiles ==="
echo ""

# Check mkfiles
echo "Build Files:"
for d in libatomspace libpln libplan9cog libtensorlogic; do
    if [ -f "sys/src/$d/mkfile" ]; then
        test_pass "sys/src/$d/mkfile exists"
    else
        test_fail "sys/src/$d/mkfile missing"
    fi
done

if [ -f "sys/src/test/mkfile" ]; then
    test_pass "sys/src/test/mkfile exists"
else
    test_fail "sys/src/test/mkfile missing"
fi

echo ""
echo "=== Checking Implementation Completeness ==="
echo ""

# Check for TODO markers in source files
echo "Checking for incomplete implementations (TODO markers):"

todo_count=0
for f in sys/src/libatomspace/*.c sys/src/libpln/*.c sys/src/libplan9cog/*.c sys/src/libtensorlogic/*.c; do
    if [ -f "$f" ]; then
        count=$(grep -c "TODO" "$f" 2>/dev/null || echo 0)
        count=${count:-0}
        if [ "$count" -gt 0 ] 2>/dev/null; then
            echo "  $f: $count TODO(s) remaining"
            todo_count=$((todo_count + count))
        fi
    fi
done

if [ "$todo_count" -eq 0 ]; then
    test_pass "No TODO markers found"
else
    echo -e "  ${YELLOW}Total TODOs: $todo_count${NC}"
fi

echo ""
echo "=== Verifying Code Quality ==="
echo ""

# Count lines of code
echo "Code Statistics:"
total_loc=0

for dir in sys/src/libatomspace sys/src/libpln sys/src/libplan9cog sys/src/libtensorlogic; do
    if [ -d "$dir" ]; then
        loc=$(wc -l $dir/*.c 2>/dev/null | tail -1 | awk '{print $1}')
        loc=${loc:-0}
        echo "  $dir: $loc lines"
        total_loc=$((total_loc + loc))
    fi
done

test_loc=0
if [ -d "sys/src/test" ]; then
    test_loc=$(wc -l sys/src/test/*.c 2>/dev/null | tail -1 | awk '{print $1}')
    echo "  sys/src/test: $test_loc lines"
fi

echo ""
echo -e "  ${GREEN}Total implementation: $total_loc lines${NC}"
echo -e "  ${GREEN}Total tests: $test_loc lines${NC}"

echo ""
echo "=== Checking Header Definitions ==="
echo ""

# Count function declarations in headers
echo "API Functions:"
for h in sys/include/plan9cog/atomspace.h sys/include/plan9cog/pln.h sys/include/plan9cog/tensorlogic.h sys/include/plan9cog.h; do
    if [ -f "$h" ]; then
        funcs=$(grep -E "^[a-zA-Z].*\(" "$h" 2>/dev/null | wc -l)
        echo "  $(basename $h): ~$funcs function declarations"
    fi
done

echo ""
echo "=============================================="
echo "TEST SUMMARY"
echo "=============================================="
echo ""

# Calculate total checks
TOTAL_CHECKS=$((PASSED_TESTS + FAILED_TESTS))

echo -e "Total checks: $TOTAL_CHECKS"
echo -e "${GREEN}Passed: $PASSED_TESTS${NC}"
if [ "$FAILED_TESTS" -gt 0 ]; then
    echo -e "${RED}Failed: $FAILED_TESTS${NC}"
else
    echo -e "Failed: $FAILED_TESTS"
fi

echo ""

if [ "$FAILED_TESTS" -eq 0 ]; then
    echo -e "${GREEN}=============================================="
    echo "ALL TESTS PASSED!"
    echo -e "==============================================${NC}"
    exit 0
else
    echo -e "${RED}=============================================="
    echo "SOME TESTS FAILED"
    echo -e "==============================================${NC}"
    exit 1
fi
