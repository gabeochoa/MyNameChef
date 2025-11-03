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
TEST_TIMEOUT=5
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
HEADLESS_MODE=true  # Default to headless mode
STREAM_OUTPUT=false # Default to capture output

# Test list - add new tests here
TESTS=(
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

echo -e "${BLUE}🧪 My Name Chef - Test Suite Runner${NC}"
echo -e "${BLUE}====================================${NC}"
echo ""

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}❌ Error: Executable not found at $EXECUTABLE${NC}"
    echo "Please build the project first with: xmake"
    exit 1
fi

# Function to run a single test
# NOTE: Tests that fail will exit immediately - they are NEVER retried.
# Each test run is a fresh game process to avoid side effects.
run_test() {
    local test_name="$1"
    local test_number="$2"
    local total="$3"
    
    echo -e "${BLUE}[$test_number/$total]${NC} Running test: ${YELLOW}$test_name${NC}"
    
    # Build headless flag
    local headless_flag=""
    if [ "$HEADLESS_MODE" = true ]; then
        headless_flag="--headless"
    fi

    if [ "$STREAM_OUTPUT" = true ]; then
        # Stream output directly
        if timeout $TEST_TIMEOUT "$EXECUTABLE" --run-test "$test_name" $headless_flag; then
            echo -e "  ${GREEN}✅ PASSED${NC} - Test completed successfully"
            return 0
        else
            local exit_code=$?
            if [ $exit_code -eq 124 ]; then
                echo -e "  ${RED}❌ TIMEOUT${NC} - Test exceeded ${TEST_TIMEOUT}s timeout"
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
        if timeout $TEST_TIMEOUT "$EXECUTABLE" --run-test "$test_name" $headless_flag > "$output_file" 2>&1; then
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
                echo -e "  ${RED}❌ TIMEOUT${NC} - Test exceeded ${TEST_TIMEOUT}s timeout"
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

# Function to run all tests
run_all_tests() {
    TOTAL_TESTS=${#TESTS[@]}
    
    echo -e "${BLUE}Found $TOTAL_TESTS tests to run${NC}"
    echo -e "${BLUE}Timeout per test: ${TEST_TIMEOUT}s${NC}"
    if [ "$HEADLESS_MODE" = true ]; then
        echo -e "${BLUE}Mode: Headless (no visible windows)${NC}"
    else
        echo -e "${BLUE}Mode: Visible windows${NC}"
    fi
    echo ""
    
    for i in "${!TESTS[@]}"; do
        local test_name="${TESTS[$i]}"
        local test_number=$((i + 1))
        
        if run_test "$test_name" "$test_number" "$TOTAL_TESTS"; then
            ((PASSED_TESTS++))
        else
            ((FAILED_TESTS++))
        fi
        
        # Small delay between tests to ensure clean state
        sleep 0.1
    done
}

# Function to show summary
show_summary() {
    echo ""
    echo -e "${BLUE}📊 Test Summary${NC}"
    echo -e "${BLUE}===============${NC}"
    echo -e "Total tests: $TOTAL_TESTS"
    echo -e "${GREEN}Passed: $PASSED_TESTS${NC}"
    echo -e "${RED}Failed: $FAILED_TESTS${NC}"
    
    if [ $FAILED_TESTS -eq 0 ]; then
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
    echo "  -h, --help     Show this help message"
    echo "  -v, --visible  Run tests with visible windows (disables headless mode)"
    echo "  -t, --timeout  Set timeout per test (default: 5s)"
    echo "  --stream       Stream test output directly (do not capture)"
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
        *)
            echo "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Main execution
run_all_tests
show_summary
