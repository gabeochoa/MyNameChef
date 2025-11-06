#!/bin/bash

# My Name Chef - Test Runner Script
# Runs all available tests with proper state reset between runs

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
EXECUTABLE="./output/my_name_chef.exe"
SERVER_EXECUTABLE="./output/battle_server.exe"
VERIFY_SCRIPT="./scripts/verify_battle_endpoint.sh"
TEST_TIMEOUT=5
SERVER_PORT=8080
TOTAL_TESTS=0
TOTAL_TESTS_RUN=0
PASSED_TESTS=0
FAILED_TESTS=0
SERVER_TESTS_PASSED=0
SERVER_TESTS_FAILED=0
BATTLE_ENDPOINT_PASSED=0
BATTLE_ENDPOINT_FAILED=0
HEADLESS_MODE=true  # Default to headless mode
STREAM_OUTPUT=false # Default to capture output
RUN_SERVER_TESTS=true
RUN_BATTLE_ENDPOINT=true

# Test groups
# Client tests that need the shared server (NetworkInfo checks)
CLIENT_TESTS=(
    "validate_main_menu"
    "validate_dish_system"
    "validate_debug_dish_creation"
    "validate_debug_dish_onserve_flavor_stats"
    "validate_debug_dish_onserve_target_scopes"
    "validate_debug_dish_onserve_combat_mods"
    "validate_debug_dish_onstartbattle"
    "validate_debug_dish_oncoursestart"
    "validate_debug_dish_onbitetaken"
    "validate_debug_dish_ondishfinished"
    "validate_debug_dish_oncoursecomplete"
    "validate_trigger_system"
    "validate_effect_system"
    "play_navigates_to_shop"
    "goto_battle"
    "validate_shop_navigation"
    "validate_shop_functionality"
    "validate_reroll_cost"
    "validate_dish_merging"
    "validate_shop_purchase"
    "validate_shop_purchase_no_gold"
    "validate_shop_purchase_insufficient_funds"
    "validate_shop_purchase_full_inventory"
    "validate_shop_purchase_exact_gold"
    "validate_shop_purchase_nonexistent_item"
    "validate_shop_purchase_wrong_screen"
    "validate_seeded_rng_determinism"
    "validate_seeded_rng_helper_methods"
    "validate_combat_system"
    "validate_battle_results"
    "validate_ui_navigation"
    "validate_full_game_flow"
)

# Integration tests that start their own server
INTEGRATION_TESTS=(
    "validate_server_battle_integration"
    "validate_server_opponent_match"
)

echo -e "${BLUE}🧪 My Name Chef - Test Suite Runner${NC}"
echo -e "${BLUE}====================================${NC}"
echo ""

# Check if executables exist
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}❌ Error: Executable not found at $EXECUTABLE${NC}"
    echo "Please build the project first with: xmake"
    exit 1
fi

if [ "$RUN_SERVER_TESTS" = true ] && [ ! -f "$SERVER_EXECUTABLE" ]; then
    echo -e "${YELLOW}⚠️  Warning: Server executable not found at $SERVER_EXECUTABLE${NC}"
    echo "Server tests will be skipped. Build with: xmake battle_server"
    RUN_SERVER_TESTS=false
fi

if [ "$RUN_BATTLE_ENDPOINT" = true ] && [ ! -f "$VERIFY_SCRIPT" ]; then
    echo -e "${YELLOW}⚠️  Warning: Verification script not found at $VERIFY_SCRIPT${NC}"
    RUN_BATTLE_ENDPOINT=false
fi

# Function to run a single test
# NOTE: Tests that fail will exit immediately - they are NEVER retried.
# Each test run is a fresh game process to avoid side effects.
run_test() {
    local test_name="$1"
    local test_number="$2"
    local total="$3"
    
    echo -e "${BLUE}[$test_number/$total]${NC} Running test: ${YELLOW}$test_name${NC}"
    
    # Integration tests must run visually (not headless) and need longer timeout
    local is_integration_test=false
    local test_timeout=$TEST_TIMEOUT
    for int_test in "${INTEGRATION_TESTS[@]}"; do
        if [ "$test_name" = "$int_test" ]; then
            is_integration_test=true
            test_timeout=60
            echo -e "${YELLOW}  ℹ️  Integration test - running in visible mode (timeout: ${test_timeout}s)${NC}"
            break
        fi
    done
    
    # Build headless flag
    local headless_flag=""
    if [ "$HEADLESS_MODE" = true ] && [ "$is_integration_test" = false ]; then
        headless_flag="--headless"
    fi

    if [ "$STREAM_OUTPUT" = true ]; then
        # Stream output directly
        if timeout $test_timeout "$EXECUTABLE" --run-test "$test_name" $headless_flag; then
            echo -e "  ${GREEN}✅ PASSED${NC} - Test completed successfully"
            return 0
        else
            local exit_code=$?
            if [ $exit_code -eq 124 ]; then
                echo -e "  ${RED}❌ TIMEOUT${NC} - Test exceeded ${test_timeout}s timeout"
            elif [ $exit_code -eq 139 ]; then
                echo -e "  ${RED}❌ SEGFAULT${NC} - Test caused segmentation fault"
            else
                echo -e "  ${RED}❌ FAILED${NC} - Test failed with exit code $exit_code"
            fi
            return 1
        fi
    else
        # Capture output to file (default)
        local output_file="/tmp/test_output_$$"
        if timeout $test_timeout "$EXECUTABLE" --run-test "$test_name" $headless_flag > "$output_file" 2>&1; then
            # Check if test completed successfully
            if grep -q "TEST COMPLETED:" "$output_file" || grep -q "TEST VALIDATION PASSED:" "$output_file" || grep -q "TEST PASSED:" "$output_file"; then
                echo -e "  ${GREEN}✅ PASSED${NC} - Test completed successfully"
                rm -f "$output_file"
                return 0
            else
                echo -e "  ${RED}❌ INCOMPLETE${NC} - Test ran but didn't complete properly"
                rm -f "$output_file"
                return 1
            fi
        else
            local exit_code=$?
            if [ $exit_code -eq 124 ]; then
                echo -e "  ${RED}❌ TIMEOUT${NC} - Test exceeded ${test_timeout}s timeout"
            elif [ $exit_code -eq 139 ]; then
                echo -e "  ${RED}❌ SEGFAULT${NC} - Test caused segmentation fault"
            else
                echo -e "  ${RED}❌ FAILED${NC} - Test failed with exit code $exit_code"
            fi
            rm -f "$output_file"
            return 1
        fi
    fi
}

# Function to start battle server for client tests
start_client_test_server() {
    if [ "$RUN_SERVER_TESTS" != true ]; then
        return 0
    fi
    
    echo -e "${BLUE}Starting battle server for client tests...${NC}"
    
    nohup "$SERVER_EXECUTABLE" > /tmp/client_test_server_$$.log 2>&1 &
    CLIENT_TEST_SERVER_PID=$!
    
    sleep 2
    
    if ! kill -0 $CLIENT_TEST_SERVER_PID 2>/dev/null; then
        echo -e "  ${RED}❌ FAILED${NC} - Server process died immediately"
        if [ -f /tmp/client_test_server_$$.log ]; then
            echo "  Server log:"
            tail -20 /tmp/client_test_server_$$.log | sed 's/^/  /'
        fi
        return 1
    fi
    
    echo -e "${BLUE}Waiting for server to start on port ${SERVER_PORT}...${NC}"
    local max_attempts=15
    local attempt=0
    while [ $attempt -lt $max_attempts ]; do
        if curl -s -f "http://localhost:${SERVER_PORT}/health" > /dev/null 2>&1; then
            echo -e "  ${GREEN}✅ Server started${NC}"
            return 0
        fi
        sleep 1
        ((attempt++))
        
        if ! kill -0 $CLIENT_TEST_SERVER_PID 2>/dev/null; then
            echo -e "  ${RED}❌ FAILED${NC} - Server process died during startup"
            if [ -f /tmp/client_test_server_$$.log ]; then
                echo "  Server log:"
                tail -20 /tmp/client_test_server_$$.log | sed 's/^/  /'
            fi
            return 1
        fi
    done
    
    if [ $attempt -eq $max_attempts ]; then
        echo -e "  ${RED}❌ FAILED${NC} - Server failed to respond on port ${SERVER_PORT}"
        kill $CLIENT_TEST_SERVER_PID 2>/dev/null || true
        return 1
    fi
}

# Function to stop battle server for client tests
stop_client_test_server() {
    if [ -z "$CLIENT_TEST_SERVER_PID" ]; then
        return 0
    fi
    
    if kill -0 $CLIENT_TEST_SERVER_PID 2>/dev/null; then
        echo -e "${BLUE}Stopping battle server...${NC}"
        kill $CLIENT_TEST_SERVER_PID 2>/dev/null || true
        wait $CLIENT_TEST_SERVER_PID 2>/dev/null || true
        rm -f /tmp/client_test_server_$$.log
    fi
}

# Function to run a group of tests
run_test_group() {
    local test_group=("$@")
    local group_total=${#test_group[@]}
    local start_number=$((TOTAL_TESTS_RUN + 1))
    
    for i in "${!test_group[@]}"; do
        local test_name="${test_group[$i]}"
        local test_number=$((start_number + i))
        
        if run_test "$test_name" "$test_number" "$TOTAL_TESTS"; then
            ((PASSED_TESTS++))
        else
            ((FAILED_TESTS++))
        fi
        
        ((TOTAL_TESTS_RUN++))
        
        # Small delay between tests to ensure clean state
        sleep 0.1
    done
}

# Function to run client tests (with server)
run_client_tests() {
    echo -e "${BLUE}Found ${#CLIENT_TESTS[@]} client tests to run${NC}"
    echo -e "${BLUE}Timeout per test: ${TEST_TIMEOUT}s${NC}"
    if [ "$HEADLESS_MODE" = true ]; then
        echo -e "${BLUE}Mode: Headless (no visible windows)${NC}"
    else
        echo -e "${BLUE}Mode: Visible windows${NC}"
    fi
    echo ""
    
    run_test_group "${CLIENT_TESTS[@]}"
}

# Function to run integration tests (without shared server)
run_integration_tests() {
    echo ""
    echo -e "${BLUE}🔗 Integration Tests${NC}"
    echo -e "${BLUE}==================${NC}"
    echo -e "${BLUE}Found ${#INTEGRATION_TESTS[@]} integration tests to run${NC}"
    echo -e "${YELLOW}Note: Integration tests start their own server and run in visible mode${NC}"
    echo ""
    
    run_test_group "${INTEGRATION_TESTS[@]}"
}

# Function to run server unit tests
run_server_tests() {
    echo ""
    echo -e "${BLUE}🔧 Server Unit Tests${NC}"
    echo -e "${BLUE}===================${NC}"
    
    if [ "$RUN_SERVER_TESTS" != true ]; then
        echo -e "${YELLOW}⚠️  Skipping server tests (executable not found)${NC}"
        return 0
    fi
    
    echo -e "${BLUE}Running server unit tests...${NC}"
    
    if timeout 30 "$SERVER_EXECUTABLE" --run-tests > /tmp/server_test_output_$$ 2>&1; then
        if grep -q "Test Results:.*passed.*0 failed" /tmp/server_test_output_$$; then
            echo -e "  ${GREEN}✅ PASSED${NC} - All server tests passed"
            SERVER_TESTS_PASSED=1
            rm -f /tmp/server_test_output_$$
            return 0
        else
            echo -e "  ${RED}❌ FAILED${NC} - Some server tests failed"
            SERVER_TESTS_FAILED=1
            echo "  Server test output:"
            tail -20 /tmp/server_test_output_$$ | sed 's/^/  /'
            rm -f /tmp/server_test_output_$$
            return 1
        fi
    else
        local exit_code=$?
        echo -e "  ${RED}❌ FAILED${NC} - Server tests failed with exit code $exit_code"
        SERVER_TESTS_FAILED=1
        if [ -f /tmp/server_test_output_$$ ]; then
            echo "  Server test output:"
            tail -20 /tmp/server_test_output_$$ | sed 's/^/  /'
            rm -f /tmp/server_test_output_$$
        fi
        return 1
    fi
}

# Function to run battle endpoint verification
run_battle_endpoint_verification() {
    echo ""
    echo -e "${BLUE}🌐 Battle Endpoint Verification${NC}"
    echo -e "${BLUE}================================${NC}"
    
    if [ "$RUN_BATTLE_ENDPOINT" != true ]; then
        echo -e "${YELLOW}⚠️  Skipping battle endpoint verification (script not found)${NC}"
        return 0
    fi
    
    if [ "$RUN_SERVER_TESTS" != true ]; then
        echo -e "${YELLOW}⚠️  Skipping battle endpoint verification (server executable not found)${NC}"
        return 0
    fi
    
    echo -e "${BLUE}Starting battle server in background...${NC}"
    
    # Start server in background with nohup to prevent crashes from script signals
    nohup "$SERVER_EXECUTABLE" > /tmp/battle_server_$$.log 2>&1 &
    SERVER_PID=$!
    
    # Wait a moment for the process to start
    sleep 2
    
    # Check if process is still running
    if ! kill -0 $SERVER_PID 2>/dev/null; then
        echo -e "  ${RED}❌ FAILED${NC} - Server process died immediately"
        if [ -f /tmp/battle_server_$$.log ]; then
            echo "  Server log:"
            tail -20 /tmp/battle_server_$$.log | sed 's/^/  /'
        fi
        BATTLE_ENDPOINT_FAILED=1
        return 1
    fi
    
    # Wait for server to start
    echo -e "${BLUE}Waiting for server to start on port ${SERVER_PORT}...${NC}"
    local max_attempts=15
    local attempt=0
    while [ $attempt -lt $max_attempts ]; do
        if curl -s -f "http://localhost:${SERVER_PORT}/health" > /dev/null 2>&1; then
            echo -e "  ${GREEN}✅ Server started${NC}"
            break
        fi
        sleep 1
        ((attempt++))
        
        # Check if process is still running
        if ! kill -0 $SERVER_PID 2>/dev/null; then
            echo -e "  ${RED}❌ FAILED${NC} - Server process died during startup"
            if [ -f /tmp/battle_server_$$.log ]; then
                echo "  Server log:"
                tail -20 /tmp/battle_server_$$.log | sed 's/^/  /'
            fi
            BATTLE_ENDPOINT_FAILED=1
            return 1
        fi
    done
    
    if [ $attempt -eq $max_attempts ]; then
        echo -e "  ${RED}❌ FAILED${NC} - Server failed to respond on port ${SERVER_PORT}"
        if [ -f /tmp/battle_server_$$.log ]; then
            echo "  Server log:"
            tail -20 /tmp/battle_server_$$.log | sed 's/^/  /'
        fi
        kill $SERVER_PID 2>/dev/null || true
        BATTLE_ENDPOINT_FAILED=1
        return 1
    fi
    
    # Run verification script
    echo -e "${BLUE}Running endpoint verification...${NC}"
    if "$VERIFY_SCRIPT" "$SERVER_PORT" > /tmp/battle_endpoint_$$.log 2>&1; then
        echo -e "  ${GREEN}✅ PASSED${NC} - All endpoint verification tests passed"
        BATTLE_ENDPOINT_PASSED=1
        rm -f /tmp/battle_endpoint_$$.log
    else
        echo -e "  ${RED}❌ FAILED${NC} - Endpoint verification failed"
        BATTLE_ENDPOINT_FAILED=1
        echo "  Verification output:"
        tail -30 /tmp/battle_endpoint_$$.log | sed 's/^/  /'
        rm -f /tmp/battle_endpoint_$$.log
    fi
    
    # Stop server
    echo -e "${BLUE}Stopping battle server...${NC}"
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    rm -f /tmp/battle_server_$$.log
    
    return 0
}

# Function to show summary
show_summary() {
    echo ""
    echo -e "${BLUE}📊 Test Summary${NC}"
    echo -e "${BLUE}===============${NC}"
    
    local total_failed=0
    
    # Server tests summary
    if [ "$RUN_SERVER_TESTS" = true ]; then
        if [ $SERVER_TESTS_FAILED -eq 0 ]; then
            echo -e "${GREEN}✅ Server Unit Tests: PASSED${NC}"
        else
            echo -e "${RED}❌ Server Unit Tests: FAILED${NC}"
            ((total_failed++))
        fi
    fi
    
    # Battle endpoint summary
    if [ "$RUN_BATTLE_ENDPOINT" = true ] && [ "$RUN_SERVER_TESTS" = true ]; then
        if [ $BATTLE_ENDPOINT_FAILED -eq 0 ]; then
            echo -e "${GREEN}✅ Battle Endpoint Verification: PASSED${NC}"
        else
            echo -e "${RED}❌ Battle Endpoint Verification: FAILED${NC}"
            ((total_failed++))
        fi
    fi
    
    # Client tests summary
    echo -e "Client Tests: ${TOTAL_TESTS} total"
    echo -e "  ${GREEN}Passed: $PASSED_TESTS${NC}"
    echo -e "  ${RED}Failed: $FAILED_TESTS${NC}"
    
    if [ $FAILED_TESTS -gt 0 ]; then
        ((total_failed++))
    fi
    
    echo ""
    
    if [ $total_failed -eq 0 ] && [ $FAILED_TESTS -eq 0 ]; then
        echo -e "${GREEN}🎉 All tests passed!${NC}"
        return 0
    else
        echo -e "${RED}⚠️  Some tests failed. Check the output above for details.${NC}"
        return 1
    fi
}

# Function to show help
show_help() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -h, --help           Show this help message"
    echo "  -v, --visible        Run tests with visible windows (disables headless mode)"
    echo "  -t, --timeout        Set timeout per test (default: 5s)"
    echo "  --stream              Stream test output directly (do not capture)"
    echo "  --no-server          Skip server unit tests"
    echo "  --no-endpoint        Skip battle endpoint verification"
    echo "  --client-only        Skip server tests and endpoint verification (client tests only)"
    echo ""
    echo "Available tests:"
    for test in "${TESTS[@]}"; do
        echo "  - $test"
    done
    echo ""
    echo "Examples:"
    echo "  $0                    # Run all tests in headless mode"
    echo "  $0 -v                 # Run all tests with visible windows"
    echo "  $0 -t 10              # Run all tests with 10s timeout"
    echo "  $0 --help             # Show this help"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -t|--timeout)
            TEST_TIMEOUT="$2"
            shift 2
            ;;
        -v|--visible)
            HEADLESS_MODE=false
            shift
            ;;
        --stream)
            STREAM_OUTPUT=true
            shift
            ;;
        --no-server)
            RUN_SERVER_TESTS=false
            shift
            ;;
        --no-endpoint)
            RUN_BATTLE_ENDPOINT=false
            shift
            ;;
        --client-only)
            RUN_SERVER_TESTS=false
            RUN_BATTLE_ENDPOINT=false
            shift
            ;;
        *)
            echo "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Main execution
echo ""

# Run server unit tests first
if [ "$RUN_SERVER_TESTS" = true ]; then
    run_server_tests
fi

# Run battle endpoint verification
if [ "$RUN_BATTLE_ENDPOINT" = true ]; then
    run_battle_endpoint_verification
fi

# Calculate total tests
TOTAL_TESTS=$((${#CLIENT_TESTS[@]} + ${#INTEGRATION_TESTS[@]}))

# Run client tests (with shared server)
echo ""
echo -e "${BLUE}🎮 Client Tests${NC}"
echo -e "${BLUE}==============${NC}"

# Start server before client tests
start_client_test_server

# Run client tests (server will be running)
run_client_tests

# Stop server after client tests, before integration tests
stop_client_test_server

# Run integration tests (they start their own server)
run_integration_tests

show_summary
