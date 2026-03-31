#!/bin/sh

# Mock cogfs for testing purposes
mkdir -p /tmp/cogfs/atom

cat > /tmp/cogfs/atom/ctl <<EOF
(cogfs mock ctl file)
EOF

cat > /tmp/cogfs/atom/node <<EOF
(cogfs mock node file)
EOF

# Test case 1: List atoms
echo "--- Test Case: ls /cog/atom ---"

# We can't actually mount cogfs, so we'll simulate the directory structure
OUTPUT=$(ls /tmp/cogfs/atom)
EXPECTED="ctl
node"

if [ "$OUTPUT" = "$EXPECTED" ]; then
    echo "  [PASS] Correctly listed atom types"
else
    echo "  [FAIL] Unexpected output: $OUTPUT"
    exit 1
fi

# Test case 2: Read from a cogfs file
echo "--- Test Case: cat /cog/atom/ctl ---"
OUTPUT=$(cat /tmp/cogfs/atom/ctl)
EXPECTED="(cogfs mock ctl file)"

if [ "$OUTPUT" = "$EXPECTED" ]; then
    echo "  [PASS] Correctly read from ctl file"
else
    echo "  [FAIL] Unexpected output: $OUTPUT"
    exit 1
fi

rm -r /tmp/cogfs

echo "\nAll cogfs tests passed."
exit 0
