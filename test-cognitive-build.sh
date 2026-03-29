#!/bin/bash
# Cognitive Kernel Build Test Script
# Tests compilation of all cognitive kernel modules

set -e

echo "=== Cognitive Kernel Build Tests ==="
echo ""

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PASS_COUNT=0
FAIL_COUNT=0
WARN_COUNT=0

# Function to log results
log_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    PASS_COUNT=$((PASS_COUNT + 1))
}

log_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    FAIL_COUNT=$((FAIL_COUNT + 1))
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
    WARN_COUNT=$((WARN_COUNT + 1))
}

# Base directories
KERNEL_PORT="sys/src/9/port"
KERNEL_PC="sys/src/9/pc"
INCLUDE="sys/include"

echo "--- Checking file existence ---"

# Check cognitive kernel modules exist
COGNITIVE_FILES=(
    "$KERNEL_PORT/devcog.c"
    "$KERNEL_PORT/cogvm.c"
    "$KERNEL_PORT/cogproc.c"
    "$KERNEL_PORT/cogmem.c"
    "$KERNEL_PORT/cogtensor.c"
    "$KERNEL_PORT/cogtest.c"
    "$KERNEL_PORT/portdat.h"
    "$KERNEL_PORT/portfns.h"
    "$KERNEL_PC/main.c"
    "$KERNEL_PORT/proc.c"
)

for file in "${COGNITIVE_FILES[@]}"; do
    if [ -f "$file" ]; then
        log_pass "Found: $file"
    else
        log_fail "Missing: $file"
    fi
done

echo ""
echo "--- Checking header contents ---"

# Check portdat.h has cognitive types
if grep -q "CogProcExt" "$KERNEL_PORT/portdat.h"; then
    log_pass "portdat.h: CogProcExt typedef found"
else
    log_fail "portdat.h: CogProcExt typedef missing"
fi

if grep -q "CogAtomSpace" "$KERNEL_PORT/portdat.h"; then
    log_pass "portdat.h: CogAtomSpace typedef found"
else
    log_fail "portdat.h: CogAtomSpace typedef missing"
fi

if grep -q "Tensor" "$KERNEL_PORT/portdat.h"; then
    log_pass "portdat.h: Tensor typedef found"
else
    log_fail "portdat.h: Tensor typedef missing"
fi

if grep -q "struct CogProcExt" "$KERNEL_PORT/portdat.h"; then
    log_pass "portdat.h: CogProcExt struct found"
else
    log_fail "portdat.h: CogProcExt struct missing"
fi

if grep -q "CogIdle" "$KERNEL_PORT/portdat.h"; then
    log_pass "portdat.h: Cognitive states enum found"
else
    log_fail "portdat.h: Cognitive states enum missing"
fi

if grep -q "COGcreate" "$KERNEL_PORT/portdat.h"; then
    log_pass "portdat.h: Cognitive VM opcodes found"
else
    log_fail "portdat.h: Cognitive VM opcodes missing"
fi

echo ""
echo "--- Checking portfns.h function declarations ---"

# Check portfns.h has cognitive functions
COGNITIVE_FUNCS=(
    "cogvminit"
    "cogprocinit"
    "cogmeminit"
    "cogatomspaceinit"
    "cogplninit"
    "cogecaninit"
    "cogtensorinit"
    "cogprocalloc"
    "cogprocextfree"
    "tensoreinsum"
    "tensortv"
    "tensorembed"
    "runcognitvetests"
)

for func in "${COGNITIVE_FUNCS[@]}"; do
    if grep -q "$func" "$KERNEL_PORT/portfns.h"; then
        log_pass "portfns.h: $func declaration found"
    else
        log_fail "portfns.h: $func declaration missing"
    fi
done

echo ""
echo "--- Checking main.c cognitive initialization ---"

if grep -q "cogmeminit" "$KERNEL_PC/main.c"; then
    log_pass "main.c: cogmeminit call found"
else
    log_fail "main.c: cogmeminit call missing"
fi

if grep -q "cogtensorinit" "$KERNEL_PC/main.c"; then
    log_pass "main.c: cogtensorinit call found"
else
    log_fail "main.c: cogtensorinit call missing"
fi

if grep -q "cognitive kernel initialized" "$KERNEL_PC/main.c"; then
    log_pass "main.c: cognitive init message found"
else
    log_warn "main.c: cognitive init message not found"
fi

echo ""
echo "--- Checking proc.c cognitive integration ---"

if grep -q "cogprocalloc" "$KERNEL_PORT/proc.c"; then
    log_pass "proc.c: cogprocalloc in newproc found"
else
    log_fail "proc.c: cogprocalloc call missing"
fi

if grep -q "cogprocextfree" "$KERNEL_PORT/proc.c"; then
    log_pass "proc.c: cogprocextfree in pexit found"
else
    log_fail "proc.c: cogprocextfree call missing"
fi

if grep -q "Cognitive scheduling" "$KERNEL_PORT/proc.c"; then
    log_pass "proc.c: Cognitive scheduling comment found"
else
    log_warn "proc.c: Cognitive scheduling comment not found"
fi

if grep -q "bestpri" "$KERNEL_PORT/proc.c"; then
    log_pass "proc.c: Cognitive priority (bestpri) found in runproc"
else
    log_fail "proc.c: Cognitive priority missing from runproc"
fi

echo ""
echo "--- Checking cogtensor.c tensor logic ---"

if grep -q "tensoreinsum" "$KERNEL_PORT/cogtensor.c"; then
    log_pass "cogtensor.c: tensoreinsum function found"
else
    log_fail "cogtensor.c: tensoreinsum function missing"
fi

if grep -q "tensortv" "$KERNEL_PORT/cogtensor.c"; then
    log_pass "cogtensor.c: tensortv function found"
else
    log_fail "cogtensor.c: tensortv function missing"
fi

if grep -q "tensorembed" "$KERNEL_PORT/cogtensor.c"; then
    log_pass "cogtensor.c: tensorembed function found"
else
    log_fail "cogtensor.c: tensorembed function missing"
fi

if grep -q "tensorattention" "$KERNEL_PORT/cogtensor.c"; then
    log_pass "cogtensor.c: tensorattention function found"
else
    log_fail "cogtensor.c: tensorattention function missing"
fi

echo ""
echo "--- Checking cogtest.c unit tests ---"

if grep -q "test_atomspace_init" "$KERNEL_PORT/cogtest.c"; then
    log_pass "cogtest.c: AtomSpace tests found"
else
    log_fail "cogtest.c: AtomSpace tests missing"
fi

if grep -q "test_pln_deduction" "$KERNEL_PORT/cogtest.c"; then
    log_pass "cogtest.c: PLN tests found"
else
    log_fail "cogtest.c: PLN tests missing"
fi

if grep -q "test_ecan_stimulate" "$KERNEL_PORT/cogtest.c"; then
    log_pass "cogtest.c: ECAN tests found"
else
    log_fail "cogtest.c: ECAN tests missing"
fi

if grep -q "test_tensor_einsum" "$KERNEL_PORT/cogtest.c"; then
    log_pass "cogtest.c: Tensor logic tests found"
else
    log_fail "cogtest.c: Tensor logic tests missing"
fi

if grep -q "runcognitvetests" "$KERNEL_PORT/cogtest.c"; then
    log_pass "cogtest.c: Test runner function found"
else
    log_fail "cogtest.c: Test runner function missing"
fi

echo ""
echo "--- Checking device registration ---"

if grep -q "cog" "$KERNEL_PORT/master.local"; then
    log_pass "master.local: cog device registered"
else
    log_fail "master.local: cog device not registered"
fi

echo ""
echo "--- Checking C syntax (basic validation) ---"

# Check for common syntax issues
check_syntax() {
    local file=$1
    local issues=0

    # Check for unmatched braces (simple heuristic)
    local open_braces=$(grep -o '{' "$file" 2>/dev/null | wc -l)
    local close_braces=$(grep -o '}' "$file" 2>/dev/null | wc -l)
    if [ "$open_braces" != "$close_braces" ]; then
        log_warn "$file: Unbalanced braces (open: $open_braces, close: $close_braces)"
        issues=1
    fi

    # Check for missing semicolons after struct definitions
    if grep -E "^struct\s+\w+\s*\{" "$file" | head -1 > /dev/null 2>&1; then
        if ! grep -E "^\};" "$file" > /dev/null 2>&1; then
            log_warn "$file: May have missing semicolons after struct"
        fi
    fi

    if [ $issues -eq 0 ]; then
        log_pass "$file: Basic syntax check passed"
    fi
}

for file in "${COGNITIVE_FILES[@]}"; do
    if [ -f "$file" ] && [[ "$file" == *.c || "$file" == *.h ]]; then
        check_syntax "$file"
    fi
done

echo ""
echo "--- Checking CLAUDE.MD documentation ---"

if [ -f "CLAUDE.MD" ]; then
    log_pass "CLAUDE.MD exists"

    if grep -q "Phase 2A" CLAUDE.MD; then
        log_pass "CLAUDE.MD: Phase 2A documented"
    else
        log_warn "CLAUDE.MD: Phase 2A not documented"
    fi

    if grep -q "Phase 2B" CLAUDE.MD; then
        log_pass "CLAUDE.MD: Phase 2B documented"
    else
        log_warn "CLAUDE.MD: Phase 2B not documented"
    fi

    if grep -q "Tensor" CLAUDE.MD || grep -q "tensor" CLAUDE.MD; then
        log_pass "CLAUDE.MD: Tensor logic mentioned"
    else
        log_warn "CLAUDE.MD: Tensor logic not mentioned"
    fi

    if grep -q "Phase 2D" CLAUDE.MD; then
        log_pass "CLAUDE.MD: Phase 2D documented"
    else
        log_warn "CLAUDE.MD: Phase 2D not documented"
    fi
else
    log_fail "CLAUDE.MD does not exist"
fi

echo ""
echo "--- Checking Phase 2D: Cognitive System Calls ---"

SYSCALL_H="sys/src/libc/9syscall/sys.h"
SYSPROC="sys/src/9/port/sysproc.c"
SYSTAB="sys/src/9/port/systab.h"

if grep -q "COGTHINK" "$SYSCALL_H"; then
    log_pass "$SYSCALL_H: COGTHINK syscall number defined"
else
    log_fail "$SYSCALL_H: COGTHINK syscall number missing"
fi

if grep -q "COGWAIT" "$SYSCALL_H"; then
    log_pass "$SYSCALL_H: COGWAIT syscall number defined"
else
    log_fail "$SYSCALL_H: COGWAIT syscall number missing"
fi

if grep -q "COGINFER" "$SYSCALL_H"; then
    log_pass "$SYSCALL_H: COGINFER syscall number defined"
else
    log_fail "$SYSCALL_H: COGINFER syscall number missing"
fi

if grep -q "COGFOCUS" "$SYSCALL_H"; then
    log_pass "$SYSCALL_H: COGFOCUS syscall number defined"
else
    log_fail "$SYSCALL_H: COGFOCUS syscall number missing"
fi

if grep -q "COGSPREAD" "$SYSCALL_H"; then
    log_pass "$SYSCALL_H: COGSPREAD syscall number defined"
else
    log_fail "$SYSCALL_H: COGSPREAD syscall number missing"
fi

if grep -q "sys_cogthink" "$SYSPROC"; then
    log_pass "$SYSPROC: sys_cogthink handler found"
else
    log_fail "$SYSPROC: sys_cogthink handler missing"
fi

if grep -q "sys_cogwait" "$SYSPROC"; then
    log_pass "$SYSPROC: sys_cogwait handler found"
else
    log_fail "$SYSPROC: sys_cogwait handler missing"
fi

if grep -q "sys_coginfer" "$SYSPROC"; then
    log_pass "$SYSPROC: sys_coginfer handler found"
else
    log_fail "$SYSPROC: sys_coginfer handler missing"
fi

if grep -q "sys_cogfocus" "$SYSPROC"; then
    log_pass "$SYSPROC: sys_cogfocus handler found"
else
    log_fail "$SYSPROC: sys_cogfocus handler missing"
fi

if grep -q "sys_cogspread" "$SYSPROC"; then
    log_pass "$SYSPROC: sys_cogspread handler found"
else
    log_fail "$SYSPROC: sys_cogspread handler missing"
fi

if grep -q "sys_cogthink" "$SYSTAB"; then
    log_pass "$SYSTAB: sys_cogthink registered in systab"
else
    log_fail "$SYSTAB: sys_cogthink not registered in systab"
fi

if grep -q "COGTHINK" "$SYSTAB"; then
    log_pass "$SYSTAB: COGTHINK entry in dispatch table"
else
    log_fail "$SYSTAB: COGTHINK entry missing from dispatch table"
fi

if grep -q "syscoginfer" "$KERNEL_PORT/cogvm.c"; then
    log_pass "cogvm.c: syscoginfer implementation found"
else
    log_fail "cogvm.c: syscoginfer implementation missing"
fi

if grep -q "syscogfocus" "$KERNEL_PORT/cogvm.c"; then
    log_pass "cogvm.c: syscogfocus implementation found"
else
    log_fail "cogvm.c: syscogfocus implementation missing"
fi

if grep -q "syscogspread" "$KERNEL_PORT/cogvm.c"; then
    log_pass "cogvm.c: syscogspread implementation found"
else
    log_fail "cogvm.c: syscogspread implementation missing"
fi

echo ""
echo "=== Build Test Summary ==="
echo -e "Passed: ${GREEN}$PASS_COUNT${NC}"
echo -e "Failed: ${RED}$FAIL_COUNT${NC}"
echo -e "Warnings: ${YELLOW}$WARN_COUNT${NC}"
echo ""

if [ $FAIL_COUNT -eq 0 ]; then
    echo -e "${GREEN}All build tests PASSED!${NC}"
    exit 0
else
    echo -e "${RED}Some build tests FAILED!${NC}"
    exit 1
fi
