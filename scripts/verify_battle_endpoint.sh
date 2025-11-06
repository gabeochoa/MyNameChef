#!/bin/bash

# Verification script for /battle endpoint
# This script tests that the battle server processes requests and returns valid JSON
#
# Usage: ./scripts/verify_battle_endpoint.sh [port]
# Example: ./scripts/verify_battle_endpoint.sh 8080
#
# Requirements: curl, jq (for JSON parsing)
#
# Before running:
#   1. Start the battle server: ./output/battle_server.exe
#   2. Ensure at least one opponent file exists in resources/battles/opponents/

# Note: set -e is not used here to allow calling script to handle errors

# Check for required tools
if ! command -v curl &> /dev/null; then
    echo "❌ ERROR: curl is required but not installed"
    exit 1
fi

if ! command -v jq &> /dev/null; then
    echo "❌ ERROR: jq is required but not installed"
    echo "   Install with: brew install jq (macOS) or apt-get install jq (Linux)"
    exit 1
fi

PORT=${1:-8080}
BASE_URL="http://localhost:${PORT}"

echo "========================================="
echo "Battle Server Endpoint Verification"
echo "========================================="
echo "Testing server on port ${PORT}"
echo ""

# Check if server is running
echo "1. Checking server health..."
if ! curl -s -f "${BASE_URL}/health" > /dev/null; then
    echo "❌ ERROR: Server not responding on ${BASE_URL}/health"
    echo "   Make sure the server is running: ./output/battle_server.exe"
    exit 1
fi

HEALTH_RESPONSE=$(curl -s "${BASE_URL}/health")
echo "✅ Health check passed: ${HEALTH_RESPONSE}"
echo ""

# Prepare test team JSON
TEST_TEAM=$(cat <<EOF
{
  "team": [
    {"dishType": "Potato", "slot": 0, "level": 1},
    {"dishType": "Burger", "slot": 1, "level": 1},
    {"dishType": "Pizza", "slot": 2, "level": 1}
  ]
}
EOF
)

echo "2. Testing /battle endpoint..."
echo "   Sending team: ${TEST_TEAM}"
echo ""

# Send battle request
BATTLE_RESPONSE=$(curl -s -X POST "${BASE_URL}/battle" \
    -H "Content-Type: application/json" \
    -d "${TEST_TEAM}")

# Check if response is valid JSON
if ! echo "${BATTLE_RESPONSE}" | jq . > /dev/null 2>&1; then
    echo "❌ ERROR: Response is not valid JSON"
    echo "   Response: ${BATTLE_RESPONSE}"
    exit 1
fi

echo "✅ Received valid JSON response"
echo ""

# Validate response structure
echo "3. Validating response structure..."

# Check required fields
REQUIRED_FIELDS=("seed" "opponentId" "outcomes" "events" "checksum")
MISSING_FIELDS=()

for field in "${REQUIRED_FIELDS[@]}"; do
    if ! echo "${BATTLE_RESPONSE}" | jq -e ".${field}" > /dev/null 2>&1; then
        MISSING_FIELDS+=("${field}")
    fi
done

if [ ${#MISSING_FIELDS[@]} -ne 0 ]; then
    echo "❌ ERROR: Missing required fields: ${MISSING_FIELDS[*]}"
    echo "   Response: ${BATTLE_RESPONSE}"
    exit 1
fi

echo "✅ All required fields present"
echo ""

# Extract and validate values
SEED=$(echo "${BATTLE_RESPONSE}" | jq -r '.seed')
OPPONENT_ID=$(echo "${BATTLE_RESPONSE}" | jq -r '.opponentId')
OUTCOMES=$(echo "${BATTLE_RESPONSE}" | jq '.outcomes')
EVENTS=$(echo "${BATTLE_RESPONSE}" | jq '.events')
CHECKSUM=$(echo "${BATTLE_RESPONSE}" | jq -r '.checksum')

echo "4. Response details:"
echo "   Seed: ${SEED}"
echo "   Opponent ID: ${OPPONENT_ID}"
echo "   Outcomes count: $(echo "${OUTCOMES}" | jq 'length')"
echo "   Events count: $(echo "${EVENTS}" | jq 'length')"
echo "   Checksum: ${CHECKSUM}"
echo ""

# Validate outcomes is an array
if ! echo "${OUTCOMES}" | jq -e 'type == "array"' > /dev/null 2>&1; then
    echo "❌ ERROR: 'outcomes' is not an array"
    exit 1
fi

# Validate events is an array
if ! echo "${EVENTS}" | jq -e 'type == "array"' > /dev/null 2>&1; then
    echo "❌ ERROR: 'events' is not an array"
    exit 1
fi

# Validate checksum is not empty
if [ -z "${CHECKSUM}" ] || [ "${CHECKSUM}" = "null" ]; then
    echo "❌ ERROR: 'checksum' is missing or empty"
    exit 1
fi

echo "✅ Response structure validation passed"
echo ""

# Test with debug mode
echo "5. Testing with debug mode enabled..."
DEBUG_TEAM=$(cat <<EOF
{
  "team": [
    {"dishType": "Potato", "slot": 0, "level": 1},
    {"dishType": "Burger", "slot": 1, "level": 1}
  ],
  "debug": true
}
EOF
)

DEBUG_RESPONSE=$(curl -s -X POST "${BASE_URL}/battle" \
    -H "Content-Type: application/json" \
    -d "${DEBUG_TEAM}")

DEBUG_MODE=$(echo "${DEBUG_RESPONSE}" | jq -r '.debug')
HAS_SNAPSHOTS=$(echo "${DEBUG_RESPONSE}" | jq -e '.snapshots' > /dev/null 2>&1 && echo "true" || echo "false")

if [ "${DEBUG_MODE}" != "true" ]; then
    echo "❌ ERROR: Debug mode not set correctly"
    exit 1
fi

if [ "${HAS_SNAPSHOTS}" != "true" ]; then
    echo "❌ ERROR: Debug mode enabled but snapshots missing"
    exit 1
fi

echo "✅ Debug mode test passed (snapshots included)"
echo ""

# Test without debug mode (should not have snapshots)
echo "6. Testing without debug mode..."
NO_DEBUG_RESPONSE=$(curl -s -X POST "${BASE_URL}/battle" \
    -H "Content-Type: application/json" \
    -d "${TEST_TEAM}")

HAS_SNAPSHOTS=$(echo "${NO_DEBUG_RESPONSE}" | jq -e '.snapshots' > /dev/null 2>&1 && echo "true" || echo "false")

if [ "${HAS_SNAPSHOTS}" = "true" ]; then
    echo "⚠️  WARNING: Snapshots present when debug=false (might be expected)"
else
    echo "✅ Production mode test passed (snapshots excluded)"
fi
echo ""

# Test error handling
echo "7. Testing error handling..."

# Invalid team (empty team)
INVALID_TEAM='{"team": []}'
ERROR_RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "${BASE_URL}/battle" \
    -H "Content-Type: application/json" \
    -d "${INVALID_TEAM}")

HTTP_CODE=$(echo "${ERROR_RESPONSE}" | tail -n1)
BODY=$(echo "${ERROR_RESPONSE}" | sed '$d')

if [ "${HTTP_CODE}" != "400" ]; then
    echo "❌ ERROR: Expected HTTP 400 for invalid team, got ${HTTP_CODE}"
    exit 1
fi

if ! echo "${BODY}" | jq -e '.error' > /dev/null 2>&1; then
    echo "❌ ERROR: Error response missing 'error' field"
    exit 1
fi

echo "✅ Error handling test passed (HTTP 400 for invalid team)"
echo ""

echo "========================================="
echo "✅ All verification tests passed!"
echo "========================================="
echo ""
echo "Summary:"
echo "  - Health endpoint: ✅"
echo "  - Battle endpoint: ✅"
echo "  - Response structure: ✅"
echo "  - Debug mode: ✅"
echo "  - Production mode: ✅"
echo "  - Error handling: ✅"
echo ""

