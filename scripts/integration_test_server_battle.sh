#!/bin/bash

# Full Integration Test: Server + Client Battle
# This test:
# 1. Starts the battle server
# 2. Ensures opponent files exist
# 3. Starts the game client (visible or headless mode)
# 4. Client makes HTTP POST request to /battle endpoint
# 5. Server simulates battle and returns result
# 6. Client loads battle files and replays the battle
# 7. Verifies the battle completes

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

SERVER_EXECUTABLE="./output/battle_server.exe"
CLIENT_EXECUTABLE="./output/my_name_chef.exe"
SERVER_PORT=8080
BASE_URL="http://localhost:${SERVER_PORT}"

# Note: Test team is now defined in the C++ test (ValidateServerBattleIntegrationTest.h)
# The client test will send the team JSON to the server

echo -e "${BLUE}🧪 Full Integration Test: Server + Client Battle${NC}"
echo -e "${BLUE}================================================${NC}"
echo ""

# Step 1: Ensure opponent files exist
echo -e "${BLUE}1. Setting up opponent files...${NC}"
mkdir -p resources/battles/opponents

# Create a couple of opponent files
if [ ! -f "resources/battles/opponents/test_opponent_1.json" ]; then
    cat > resources/battles/opponents/test_opponent_1.json <<EOF
{
  "team": [
    {"dishType": "Ramen", "slot": 0, "level": 1},
    {"dishType": "Sushi", "slot": 1, "level": 1},
    {"dishType": "Steak", "slot": 2, "level": 1}
  ]
}
EOF
    echo -e "  ${GREEN}✅ Created test_opponent_1.json${NC}"
fi

if [ ! -f "resources/battles/opponents/test_opponent_2.json" ]; then
    cat > resources/battles/opponents/test_opponent_2.json <<EOF
{
  "team": [
    {"dishType": "GarlicBread", "slot": 0, "level": 1},
    {"dishType": "Potato", "slot": 1, "level": 1},
    {"dishType": "Burger", "slot": 2, "level": 1}
  ]
}
EOF
    echo -e "  ${GREEN}✅ Created test_opponent_2.json${NC}"
fi

OPPONENT_COUNT=$(ls -1 resources/battles/opponents/*.json 2>/dev/null | wc -l | tr -d ' ')
echo -e "  ${GREEN}✅ Found ${OPPONENT_COUNT} opponent files${NC}"
echo ""

# Step 2: Start server
echo -e "${BLUE}2. Starting battle server...${NC}"
if [ ! -f "$SERVER_EXECUTABLE" ]; then
    echo -e "  ${RED}❌ Server executable not found: $SERVER_EXECUTABLE${NC}"
    echo "  Build with: xmake battle_server"
    exit 1
fi

# Start server in background
"$SERVER_EXECUTABLE" > /tmp/integration_server_$$.log 2>&1 &
SERVER_PID=$!

# Wait for server to start
echo -e "${BLUE}   Waiting for server to start...${NC}"
max_attempts=15
attempt=0
while [ $attempt -lt $max_attempts ]; do
    if curl -s -f "${BASE_URL}/health" > /dev/null 2>&1; then
        echo -e "  ${GREEN}✅ Server started on port ${SERVER_PORT}${NC}"
        break
    fi
    sleep 1
    ((attempt++))
done

if [ $attempt -eq $max_attempts ]; then
    echo -e "  ${RED}❌ Server failed to start${NC}"
    if [ -f /tmp/integration_server_$$.log ]; then
        echo "  Server log:"
        tail -20 /tmp/integration_server_$$.log | sed 's/^/    /'
    fi
    kill $SERVER_PID 2>/dev/null || true
    exit 1
fi
echo ""

# Step 3: Run client integration test (client will request battle from server)
echo -e "${BLUE}3. Running client integration test...${NC}"
if [ ! -f "$CLIENT_EXECUTABLE" ]; then
    echo -e "  ${RED}❌ Client executable not found: $CLIENT_EXECUTABLE${NC}"
    echo "  Build with: xmake"
    exit 1
fi

# Pass server URL to client test
export INTEGRATION_SERVER_URL="${BASE_URL}"

# Check if we should run automated test or visible mode
if [ "${AUTO_TEST:-false}" = "true" ]; then
    echo -e "${BLUE}   Running automated test (headless)...${NC}"
    echo -e "${YELLOW}   Client will request battle from server at ${BASE_URL}${NC}"
    if timeout 60 "$CLIENT_EXECUTABLE" --run-test validate_server_battle_integration --headless --animation-speed-multiplier 5 > /tmp/integration_client_$$.log 2>&1; then
        echo -e "  ${GREEN}✅ Client integration test passed${NC}"
    else
        echo -e "  ${RED}❌ Client integration test failed${NC}"
        echo "  Test output:"
        tail -30 /tmp/integration_client_$$.log | sed 's/^/    /'
        rm -f /tmp/integration_client_$$.log
        kill $SERVER_PID 2>/dev/null || true
        exit 1
    fi
    rm -f /tmp/integration_client_$$.log
else
    echo -e "  ${YELLOW}ℹ️  Running client integration test in visible mode${NC}"
    echo -e "  ${YELLOW}ℹ️  Client will request battle from server at ${BASE_URL}${NC}"
    echo ""
    echo -e "${BLUE}   Starting client with test...${NC}"
    echo -e "${YELLOW}   Note: This will open a visible window.${NC}"
    echo -e "${YELLOW}   The client will request a battle from the server and load it automatically.${NC}"
    echo ""
    
    # Run integration tests in visible mode (without --headless flag)
    echo -e "${BLUE}   Running validate_server_battle_integration test...${NC}"
    "$CLIENT_EXECUTABLE" --run-test validate_server_battle_integration --animation-speed-multiplier 5
    TEST_EXIT_CODE=$?
    
    if [ $TEST_EXIT_CODE -eq 0 ]; then
        echo -e "  ${GREEN}✅ Battle integration test passed${NC}"
    else
        echo -e "  ${RED}❌ Battle integration test failed (exit code: $TEST_EXIT_CODE)${NC}"
        kill $SERVER_PID 2>/dev/null || true
        exit 1
    fi
    
    echo ""
    echo -e "${BLUE}   Running validate_server_opponent_match test...${NC}"
    "$CLIENT_EXECUTABLE" --run-test validate_server_opponent_match --animation-speed-multiplier 5
    OPPONENT_TEST_EXIT_CODE=$?
    
    if [ $OPPONENT_TEST_EXIT_CODE -eq 0 ]; then
        echo -e "  ${GREEN}✅ Opponent match test passed${NC}"
    else
        echo -e "  ${RED}❌ Opponent match test failed (exit code: $OPPONENT_TEST_EXIT_CODE)${NC}"
        kill $SERVER_PID 2>/dev/null || true
        exit 1
    fi
    echo ""
fi

# Step 4: Stop server
echo -e "${BLUE}4. Stopping server...${NC}"
kill $SERVER_PID 2>/dev/null || true
wait $SERVER_PID 2>/dev/null || true
rm -f /tmp/integration_server_$$.log
echo -e "  ${GREEN}✅ Server stopped${NC}"
echo ""

echo -e "${GREEN}✅ Integration test complete!${NC}"
echo ""
echo -e "${BLUE}📋 Test Summary${NC}"
echo -e "${BLUE}==============${NC}"
echo -e "${GREEN}✅ Server started on port ${SERVER_PORT}${NC}"
echo -e "${GREEN}✅ Client requested battle from server${NC}"
echo -e "${GREEN}✅ Client loaded and ran battle${NC}"
echo ""
echo -e "${BLUE}What happened:${NC}"
echo "  1. Server started and is ready for battle requests"
echo "  2. Client test made HTTP POST request to ${BASE_URL}/battle"
echo "  3. Server simulated battle and returned seed/result"
echo "  4. Client loaded battle files and replayed the battle"
echo "  5. Battle completed successfully"
echo ""

