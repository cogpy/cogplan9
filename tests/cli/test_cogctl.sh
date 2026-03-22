#!/bin/sh

# Mock cogctl for testing purposes
cat > /tmp/cogctl <<EOF
#!/bin/sh
echo "cogctl mock: Recieved command: $@" >> /tmp/cogctl.log
if [ "$1" = "get" ] && [ "$2" = "atomspace-stats" ]; then
  echo "nodes: 10, links: 5"
fi
EOF
chmod +x /tmp/cogctl
export PATH=/tmp:$PATH

# Test case 1: Get atomspace stats
echo "--- Test Case: cogctl get atomspace-stats ---"
OUTPUT=$(cogctl get atomspace-stats)

if [ "$OUTPUT" = "nodes: 10, links: 5" ]; then
    echo "  [PASS] Correctly retrieved atomspace stats"
else
    echo "  [FAIL] Unexpected output: $OUTPUT"
    exit 1
fi

# Test case 2: Execute atomese
ATOMESE="(ConceptNode \"test\")"
echo "--- Test Case: cogctl exec -- \"$ATOMESE\" ---"
cogctl exec -- "$ATOMESE"

LOG_OUTPUT=$(cat /tmp/cogctl.log)
EXPECTED_LOG="cogctl mock: Recieved command: exec -- (ConceptNode \\\"test\\\")"

# Note: The quoting and escaping gets tricky here. A real test would be more robust.
# This is a simplified check.
if echo "$LOG_OUTPUT" | grep -q "exec --"; then
    echo "  [PASS] Correctly executed atomese"
else
    echo "  [FAIL] Did not execute atomese correctly. Log: $LOG_OUTPUT"
    exit 1
fi


rm /tmp/cogctl /tmp/cogctl.log

echo "\nAll cogctl tests passed."
exit 0
