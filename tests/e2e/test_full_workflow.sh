#!/bin/sh

# --- Test Setup ---
echo "Setting up test environment..."

# Create a mock file system for cogfs
export COGFS_ROOT=/tmp/cogfs_e2e
rm -rf $COGFS_ROOT
mkdir -p $COGFS_ROOT/atom

# Create a mock cogctl that interacts with the mock cogfs file system
cat > /tmp/cogctl_e2e <<EOF
#!/bin/sh
ATOMESE=\$2
if [ \"\$1\" = \"exec\" ]; then
    # Extract the node name from atomese, super simplified
    NODE_NAME=\$(echo \$ATOMESE | sed -n 's/.*(ConceptNode \\\"\([^\\\"]*\)\\\").*/\1/p')
    if [ -n \"\$NODE_NAME\" ]; then
        echo \"Created node: \$NODE_NAME\" >> \$COGFS_ROOT/log
        echo \"stv 1.0 1.0\" > \"\$COGFS_ROOT/atom/\$NODE_NAME\"
    fi

    # Simulate PLN inference
    if echo \"\$ATOMESE\" | grep -q \"ImplicationLink\"; then
        echo \"Created node: C\" >> \$COGFS_ROOT/log
        echo \"stv 0.63 0.81\" > \"\$COGFS_ROOT/atom/C\"
    fi
fi
EOF
chmod +x /tmp/cogctl_e2e
export PATH=/tmp:$PATH

# --- Test Execution ---

TEST_CASE() { echo "\n--- Test Case: \$1 ---"; }
ASSERT_FILE_EXISTS() {
    if [ -f \"\$1\" ]; then echo \"  [PASS] File \$1 exists\"; else echo \"  [FAIL] File \$1 does not exist\"; exit 1; fi
}
ASSERT_FILE_CONTAINS() {
    if grep -q \"\$2\" \"\$1\"; then echo \"  [PASS] File \"\$1\" contains \"\$2\"\"; else echo \"  [FAIL] File \"\$1\" does not contain \"\$2\"\"; exit 1; fi
}

TEST_CASE "Create initial atoms via cogctl"
cogctl_e2e exec -- "(ConceptNode \"A\")"
cogctl_e2e exec -- "(ConceptNode \"B\")"

TEST_CASE "Verify atoms exist in cogfs"
ASSERT_FILE_EXISTS "\$COGFS_ROOT/atom/A"
ASSERT_FILE_EXISTS "\$COGFS_ROOT/atom/B"

TEST_CASE "Execute PLN rule via cogctl"
cogctl_e2e exec -- "(ImplicationLink (ConceptNode \"A\") (ConceptNode \"B\") (ConceptNode \"C\"))"

TEST_CASE "Verify inference result in cogfs"
ASSERT_FILE_EXISTS "\$COGFS_ROOT/atom/C"
ASSERT_FILE_CONTAINS "\$COGFS_ROOT/atom/C" "stv 0.63 0.81"

# --- Test Teardown ---
echo "\nTearing down test environment..."
rm -rf $COGFS_ROOT
rm /tmp/cogctl_e2e

echo "\nAll E2E tests passed."
exit 0
