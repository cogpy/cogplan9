#!/bin/bash
# Plan9Cog integration test script (bash version)

fail() {
    echo "FAIL: $*"
    exit 1
}

pass() {
    echo "PASS: $*"
}

echo '=== Plan9Cog Integration Tests ==='
echo ''

# Test 1: Check headers exist
echo 'Test 1: Checking headers...'
if [ -f sys/include/plan9cog.h ]; then
    pass 'Main header exists'
else
    fail 'Main header missing'
fi

if [ -f sys/include/plan9cog/atomspace.h ]; then
    pass 'AtomSpace header exists'
else
    fail 'AtomSpace header missing'
fi

if [ -f sys/include/plan9cog/pln.h ]; then
    pass 'PLN header exists'
else
    fail 'PLN header missing'
fi

if [ -f sys/include/plan9cog/cogvm.h ]; then
    pass 'CogVM header exists'
else
    fail 'CogVM header missing'
fi

echo ''

# Test 2: Check library source exists
echo 'Test 2: Checking library sources...'
if [ -d sys/src/libatomspace ]; then
    pass 'libatomspace directory exists'
else
    fail 'libatomspace directory missing'
fi

if [ -d sys/src/libpln ]; then
    pass 'libpln directory exists'
else
    fail 'libpln directory missing'
fi

if [ -d sys/src/libplan9cog ]; then
    pass 'libplan9cog directory exists'
else
    fail 'libplan9cog directory missing'
fi

echo ''

# Test 3: Check command sources exist
echo 'Test 3: Checking command sources...'
if [ -d sys/src/cmd/cogfs ]; then
    pass 'cogfs command exists'
else
    fail 'cogfs command missing'
fi

if [ -d sys/src/cmd/cogctl ]; then
    pass 'cogctl command exists'
else
    fail 'cogctl command missing'
fi

if [ -d sys/src/cmd/cogdemo ]; then
    pass 'cogdemo command exists'
else
    fail 'cogdemo command missing'
fi

echo ''

# Test 4: Check documentation
echo 'Test 4: Checking documentation...'
if [ -f PLAN9COG_GUIDE.md ]; then
    pass 'Integration guide exists'
else
    fail 'Integration guide missing'
fi

if [ -f PLAN9COG_README.md ]; then
    pass 'README exists'
else
    fail 'README missing'
fi

if [ -f ARCHITECTURE.md ]; then
    pass 'Architecture document exists'
else
    fail 'Architecture document missing'
fi

echo ''

# Test 5: Verify mkfiles
echo 'Test 5: Checking mkfiles...'
if [ -f sys/src/libatomspace/mkfile ]; then
    pass 'libatomspace mkfile exists'
else
    fail 'libatomspace mkfile missing'
fi

if [ -f sys/src/libpln/mkfile ]; then
    pass 'libpln mkfile exists'
else
    fail 'libpln mkfile missing'
fi

if [ -f sys/src/libplan9cog/mkfile ]; then
    pass 'libplan9cog mkfile exists'
else
    fail 'libplan9cog mkfile missing'
fi

echo ''

# Test 6: Check implementation files
echo 'Test 6: Checking implementation files...'
if [ -f sys/src/libatomspace/atomspace.c ]; then
    pass 'atomspace.c exists'
else
    fail 'atomspace.c missing'
fi

if [ -f sys/src/libatomspace/pattern.c ]; then
    pass 'pattern.c exists'
else
    fail 'pattern.c missing'
fi

if [ -f sys/src/libpln/pln.c ]; then
    pass 'pln.c exists'
else
    fail 'pln.c missing'
fi

if [ -f sys/src/libpln/ure.c ]; then
    pass 'ure.c exists'
else
    fail 'ure.c missing'
fi

if [ -f sys/src/libplan9cog/plan9cog.c ]; then
    pass 'plan9cog.c exists'
else
    fail 'plan9cog.c missing'
fi

if [ -f sys/src/libplan9cog/ecan.c ]; then
    pass 'ecan.c exists'
else
    fail 'ecan.c missing'
fi

if [ -f sys/src/libplan9cog/machspace.c ]; then
    pass 'machspace.c exists'
else
    fail 'machspace.c missing'
fi

echo ''

# Test 7: Check man pages
echo 'Test 7: Checking man pages...'
if [ -f sys/man/1/cogctl ]; then
    pass 'cogctl man page exists'
else
    fail 'cogctl man page missing'
fi

if [ -f sys/man/1/cogfs ]; then
    pass 'cogfs man page exists'
else
    fail 'cogfs man page missing'
fi

if [ -f sys/man/1/cogdemo ]; then
    pass 'cogdemo man page exists'
else
    fail 'cogdemo man page missing'
fi

if [ -f sys/man/2/plan9cog ]; then
    pass 'plan9cog man page exists'
else
    fail 'plan9cog man page missing'
fi

echo ''
echo '=== All Tests Passed ==='
echo ''
echo 'Plan9Cog components are properly installed.'
echo ''
echo 'Component Summary:'
echo '  - 4 header files (plan9cog.h, atomspace.h, pln.h, cogvm.h)'
echo '  - 3 libraries (libatomspace, libpln, libplan9cog)'
echo '  - 3 commands (cogctl, cogfs, cogdemo)'
echo '  - 4 man pages'
echo '  - 3 documentation files'
echo ''
echo 'To build on a real Plan 9 system, run:'
echo '  cd /sys/src/libatomspace && mk install'
echo '  cd /sys/src/libpln && mk install'
echo '  cd /sys/src/libplan9cog && mk install'
echo '  cd /sys/src/cmd/cogctl && mk install'
echo '  cd /sys/src/cmd/cogfs && mk install'
echo '  cd /sys/src/cmd/cogdemo && mk install'

exit 0
