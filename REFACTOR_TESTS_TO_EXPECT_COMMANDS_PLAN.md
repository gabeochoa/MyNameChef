# Plan: Remove Conditionals from Tests and Replace with Expect Commands

## Overview
Refactor all test files to remove conditional statements (`if`, ternary operators) and replace them with explicit `expect_*` commands and `wait_*` methods. This will make tests more declarative, easier to read, and provide better error messages.

## Current State Analysis

### Conditional Patterns Found in Tests

1. **Screen Navigation Checks**
   - `if (current_screen != expected)` → Replace with `expect_screen_is()` or `wait_for_screen()`

2. **Collection Empty Checks**
   - `if (collection.empty())` → Replace with `expect_not_empty()` or `expect_count_gt(0)`

3. **Count/Value Comparisons**
   - `if (count != expected)` → Replace with `expect_count_eq()`, `expect_count_gt()`, `expect_count_lt()`
   - `if (gold < price)` → Replace with `expect_wallet_at_least()` or `expect_wallet_has()`

4. **Component Existence Checks**
   - `if (!entity.has<Component>())` → Replace with `expect_entity_has_component()`
   - `if (!singleton.has<Component>())` → Replace with `expect_singleton_has_component()`

5. **State Phase Checks**
   - `if (phase == Phase::X)` → Replace with `expect_dish_phase()` or `expect_battle_phase()`

6. **Static State Tracking**
   - Static variables with conditional checks for state transitions → Refactor to linear flow with `wait_*` and `expect_*`

7. **Retry/Timeout Logic**
   - Static counters with `if (count < max)` conditionals → Replace with proper `wait_*` methods that handle timeouts

8. **Boolean Flags for Flow Control**
   - `if (!flag)` patterns with static flags → Replace with explicit state checks using `expect_*`

## New Expect Methods Needed

Add these methods to `TestApp` in `src/testing/test_app.h`:

```cpp
// Count expectations
TestApp &expect_count_eq(int actual, int expected, const std::string &description, const std::string &location = "");
TestApp &expect_count_gt(int actual, int min, const std::string &description, const std::string &location = "");
TestApp &expect_count_lt(int actual, int max, const std::string &description, const std::string &location = "");
TestApp &expect_count_gte(int actual, int min, const std::string &description, const std::string &location = "");
TestApp &expect_count_lte(int actual, int max, const std::string &description, const std::string &location = "");

// Collection expectations
TestApp &expect_not_empty(const auto &collection, const std::string &description, const std::string &location = "");
TestApp &expect_empty(const auto &collection, const std::string &description, const std::string &location = "");

// Entity expectations
template <typename T>
TestApp &expect_entity_has_component(afterhours::EntityID entity_id, const std::string &location = "");
TestApp &expect_singleton_has_component(auto &singleton_opt, const std::string &component_name, const std::string &location = "");

// Battle state expectations
TestApp &expect_dish_phase(afterhours::EntityID dish_id, DishBattleState::Phase expected_phase, const std::string &location = "");
TestApp &expect_dish_count(int expected_player, int expected_opponent, const std::string &location = "");
TestApp &expect_player_dish_count(int expected, const std::string &location = "");
TestApp &expect_opponent_dish_count(int expected, const std::string &location = "");
TestApp &expect_dish_count_at_least(int min_player, int min_opponent, const std::string &location = "");

// Wallet expectations
TestApp &expect_wallet_at_least(int min_gold, const std::string &location = "");
TestApp &expect_wallet_between(int min_gold, int max_gold, const std::string &location = "");

// Screen expectations (already exists but ensure usage)
// TestApp &expect_screen_is(GameStateManager::Screen screen, const std::string &location = "");

// Wait methods for state transitions
TestApp &wait_for_battle_initialized(float timeout_sec = 10.0f, const std::string &location = "");
TestApp &wait_for_dishes_in_combat(int min_count = 1, float timeout_sec = 10.0f, const std::string &location = "");
TestApp &wait_for_battle_complete(float timeout_sec = 60.0f, const std::string &location = "");
TestApp &wait_for_results_screen(float timeout_sec = 10.0f, const std::string &location = "");
```

## Implementation Steps

### Step 1: Add New Expect Methods to TestApp

**Files to modify:**
- `src/testing/test_app.h` - Add method declarations
- `src/testing/test_app.cpp` - Implement methods

**Validation:**
```bash
cd /Users/gabe/p/my_name_chef
xmake
# Expected: Build succeeds with no compilation errors
```

**Expected logs:**
- Build completes successfully
- No undefined reference errors

---

### Step 2: Refactor ValidateDishOrderingTest.h

**File:** `src/testing/tests/ValidateDishOrderingTest.h`

**Changes:**
1. Remove `if (current_screen != GameStateManager::Screen::Shop)` → Use `wait_for_screen()` before `expect_screen_is()`
2. Remove `if (battle_screen != GameStateManager::Screen::Battle)` → Use `wait_for_screen()` before `expect_screen_is()`
3. Replace static `transition_wait_count` conditional → Use `wait_for_frames()` explicitly
4. Replace static `init_check_count` and `init_logged_start` conditionals → Use `wait_for_battle_initialized()`
5. Replace phase counting conditionals → Use `expect_dish_count_at_least()` or `wait_for_dishes_in_combat()`
6. Replace `if (!combat_queue_opt.get().has<CombatQueue>())` → Use `expect_singleton_has_component()`
7. Replace `if (player_slots.empty() || opponent_slots.empty())` → Use `expect_count_gt()` for both
8. Replace slot comparison conditionals → Use `expect_count_eq()` for slot validation
9. Replace `if (cq_tracking.current_index != last_course_index)` → Use `wait_for_course_transition()` helper or refactor to linear flow
10. Replace `if (player_dish && opponent_dish)` checks → Use `expect_entity_exists()` methods
11. Replace static `wait_iteration` timeout → Use `wait_for_battle_complete()` with timeout
12. Replace `if (courses_in_order)` conditional → Use `expect_courses_in_order()` or linear validation

**Validation:**
```bash
cd /Users/gabe/p/my_name_chef
./output/my_name_chef.exe --run-test validate_dish_ordering --headless
# Expected: Test passes with same behavior, no conditionals remain
```

**Expected logs:**
- Test logs show expect calls instead of conditional checks
- Test passes successfully
- No `if` statements in the test function body (only in helper methods if needed)

---

### Step 3: Refactor ValidateFullBattleFlowTest.h

**File:** `src/testing/tests/ValidateFullBattleFlowTest.h`

**Changes:**
1. Remove `if (current_screen != GameStateManager::Screen::Shop)` → Use `wait_for_screen()` before `expect_screen_is()`
2. Remove `if (inventory.empty())` → Use `expect_not_empty()` or create item first with `create_inventory_item()`
3. Remove `if (battle_screen != GameStateManager::Screen::Battle)` → Use `wait_for_screen()` before `expect_screen_is()`
4. Replace static `transition_wait_count` conditional → Use `wait_for_frames()` explicitly
5. Replace static `init_check_count` and `init_logged_start` conditionals → Use `wait_for_battle_initialized()`
6. Replace phase counting conditionals → Use `expect_dish_phase()` or `wait_for_dishes_in_combat()`
7. Replace `if (init_check_count >= 2000)` timeout → Use `wait_for_battle_initialized()` with timeout
8. Replace static `dishes_counted` flag → Use `wait_for_dishes_counted()` or linear flow
9. Replace `if (current != GameStateManager::Screen::Battle)` → Use `expect_screen_is()` immediately after wait
10. Replace `if (initial_player_dishes == 0 && initial_opponent_dishes == 0)` → Use `expect_dish_count_at_least()` or accept as valid state
11. Replace static `results_reached` flag → Use `wait_for_results_screen()` or `wait_for_battle_complete()`
12. Replace `if (!team_emptied && (player_count == 0 || opponent_count == 0))` → Use `expect_dish_count_eq()` when appropriate
13. Replace `if (current == GameStateManager::Screen::Results)` → Use `wait_for_results_screen()` then `expect_screen_is()`
14. Replace `if (final_player > 0 && final_opponent > 0)` → Use `expect_dish_count()` or remove as test may accept this
15. Replace `if (wait_iteration >= max_iterations)` → Use `wait_for_battle_complete()` with timeout
16. Replace `if (final_player > initial_player_dishes)` → Use `expect_count_lte(final_player, initial_player_dishes)`
17. Replace `if (result_entity.get().has<BattleResult>())` → Use `expect_singleton_has_component()`
18. Replace `if (result.outcome == BattleResult::Outcome::Tie)` → Use `expect_battle_not_tie()` or `expect_battle_outcome()`
19. Replace `if (result.playerWins == 0 && result.opponentWins == 0)` → Use `expect_count_gt()` for wins

**Validation:**
```bash
cd /Users/gabe/p/my_name_chef
./output/my_name_chef.exe --run-test validate_full_battle_flow --headless
# Expected: Test passes with same behavior, no conditionals remain
```

**Expected logs:**
- Test logs show expect calls instead of conditional checks
- Test passes successfully
- No `if` statements in the test function body

---

### Step 4: Refactor ValidateShopPurchaseTest.h

**File:** `src/testing/tests/ValidateShopPurchaseTest.h`

**Changes:**
1. Replace `if (shop_items.empty())` → Use `expect_not_empty(shop_items, "shop items")`
2. Replace `if (initial_gold < item_price)` → Use `expect_wallet_at_least(item_price)`
3. Replace `if (new_gold != initial_gold - item_price)` → Use `expect_wallet_has(initial_gold - item_price)`
4. Replace `if (!found_in_inventory)` → Use `expect_inventory_contains(item_type)`
5. Replace `if (still_in_shop)` → Use `expect_shop_not_contains(item_to_buy.id)` or `expect_count_eq()` after filtering

**Validation:**
```bash
cd /Users/gabe/p/my_name_chef
./output/my_name_chef.exe --run-test validate_shop_purchase --headless
# Expected: Test passes with same behavior, no conditionals remain
```

**Expected logs:**
- Test passes successfully
- No `if` statements in the test function body

---

### Step 5: Refactor ValidateCombatSystemTest.h

**File:** `src/testing/tests/ValidateCombatSystemTest.h`

**Changes:**
1. Remove `if (gsm.active_screen == GameStateManager::Screen::Battle)` → Use `wait_for_screen()` and `expect_screen_is()` at start, or refactor to always start from known state

**Validation:**
```bash
cd /Users/gabe/p/my_name_chef
./output/my_name_chef.exe --run-test validate_combat_system --headless
# Expected: Test passes with same behavior, no conditionals remain
```

**Expected logs:**
- Test passes successfully
- No `if` statements in the test function body

---

### Step 6: Refactor Remaining Test Files

**Files to check and refactor:**
- `ValidateBattleResultsTest.h`
- `ValidateShopPurchaseNonexistentItemTest.h`
- `ValidateShopPurchaseNoGoldTest.h`
- `ValidateShopPurchaseFullInventoryTest.h`
- `ValidateShopPurchaseWrongScreenTest.h`
- `ValidateShopPurchaseExactGoldTest.h`
- `ValidateShopPurchaseInsufficientFundsTest.h`
- `ValidateDishSystemTest.h`
- `ValidateDebugDishTest.h`
- `ValidateEffectSystemTest.h`
- `ValidateTriggerSystemTest.h`
- `ValidateFullGameFlowTest.h`
- `ValidateUINavigationTest.h`
- `GotoBattleTest.h`
- `PlayNavigatesToShopTest.h`
- `ValidateMainMenuTest.h`
- `ValidateShopNavigationTest.h`
- `ValidateShopFunctionalityTest.h`

**Pattern to follow:**
- For each file, identify all `if` statements
- Replace with appropriate `expect_*` or `wait_*` methods
- Remove static state tracking variables where possible
- Convert conditional flow to linear flow with explicit waits

**Validation:**
```bash
cd /Users/gabe/p/my_name_chef
./scripts/run_all_tests.sh
# Expected: All tests pass, grep shows no conditionals in test bodies
```

**Expected logs:**
- All tests pass
- No `if` statements found in test function bodies (check with grep)

---

### Step 7: Verify No Conditionals Remain

**Validation:**
```bash
cd /Users/gabe/p/my_name_chef
grep -r "if (" src/testing/tests/*.h | grep -v "//" | grep -v "expect_" | grep -v "wait_" | wc -l
# Expected: 0 (or very few for edge cases that must remain)
```

**Expected logs:**
- Count shows 0 or minimal conditionals (only in helper functions or unavoidable cases)

---

### Step 8: Run Full Test Suite

**Validation:**
```bash
cd /Users/gabe/p/my_name_chef
./scripts/run_all_tests.sh
# Expected: All tests pass
```

**Expected logs:**
- All tests pass
- No test failures
- Test output shows expect calls in logs instead of conditional checks

---

## Summary

After completing this plan:
- All test files will use explicit `expect_*` and `wait_*` methods
- No conditional statements (`if`, ternary) in test function bodies
- Tests will be more declarative and easier to read
- Error messages will be more descriptive (from expect methods)
- Test flow will be linear instead of using static state tracking

## Notes

- Some helper functions may still contain conditionals (e.g., in `TestApp` implementation), which is acceptable
- Static variables for state tracking should be eliminated in favor of linear flow with `wait_*` methods
- Timeout logic should be handled by `wait_*` methods, not manual counters
- The goal is to make test code read like a specification rather than imperative code
