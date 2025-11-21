#include "TestSystem.h"

// Include rl.h first to ensure Vector2Type is defined as raylib::Vector2
// before any afterhours UI headers are included (fixes Vector2Type conversion
// issues)
#include "../rl.h" // NOLINT - required for Vector2Type fix

#include "../testing/test_app.h"
#include "../testing/test_macros.h"
#include <algorithm>
#include <string>
#include <unordered_map>

// Storage for test state (test app and continuation) keyed by test name
std::unordered_map<std::string, TestApp *> g_test_apps;
std::unordered_map<std::string, std::function<void()>> g_test_continuations;

// Include all test files so TEST() macro registrations run
// Static TestRegistrar_##name objects will register themselves during static
// initialization
#include "../testing/tests/GotoBattleTest.h"
#include "../testing/tests/PlayNavigatesToShopTest.h"
#include "../testing/tests/ValidateBattleResultsTest.h"
#include "../testing/tests/ValidateBattleSlotCompactionTest.h"
#include "../testing/tests/ValidateCombatSystemTest.h"
#include "../testing/tests/ValidateDebugDishTest.h"
#include "../testing/tests/ValidateDishLevelContributionTest.h"
#include "../testing/tests/ValidateDishMergingTest.h"
#include "../testing/tests/ValidateDishOrderingTest.h"
#include "../testing/tests/ValidateDishSellingTest.h"
#include "../testing/tests/ValidateDishSystemTest.h"
#include "../testing/tests/ValidateDoubleBattleFlowTest.h"
#include "../testing/tests/ValidateDrinkEffectsTest.h"
#include "../testing/tests/ValidateDrinkShopTest.h"
#include "../testing/tests/ValidateEffectSystemTest.h"
#include "../testing/tests/ValidateEntityQueryWithoutManualMergeTest.h"
#include "../testing/tests/ValidateFullBattleFlowTest.h"
#include "../testing/tests/ValidateFullGameFlowTest.h"
#include "../testing/tests/ValidateGameStateMultiRoundPersistenceTest.h"
#include "../testing/tests/ValidateGameStateNewTeamVsContinueTest.h"
#include "../testing/tests/ValidateGameStateSaveResumeBasicTest.h"
#include "../testing/tests/ValidateGameStateServerChecksumSyncTest.h"
#include "../testing/tests/ValidateGameStateShopSeedDeterminismTest.h"
#include "../testing/tests/ValidateMainMenuTest.h"
#include "../testing/tests/ValidateNewEffectOperationsTest.h"
#include "../testing/tests/ValidateReplayPausePlayTest.h"
#include "../testing/tests/ValidateRerollCostTest.h"
#include "../testing/tests/ValidateSeededRngTest.h"
#include "../testing/tests/ValidateServerBattleIntegrationTest.h"
#include "../testing/tests/ValidateServerChecksumMatchTest.h"
#include "../testing/tests/ValidateCodeHashTest.h"
#include "../testing/tests/ValidateServerFailureDuringBattleTest.h"
#include "../testing/tests/ValidateServerFailureDuringPendingRequestTest.h"
#include "../testing/tests/ValidateServerFailureDuringShopTest.h"
#include "../testing/tests/ValidateServerOpponentMatchTest.h"
#include "../testing/tests/ValidateSetBonusSystemTest.h"
#include "../testing/tests/ValidateShopFreezeBattlePersistenceTest.h"
#include "../testing/tests/ValidateSurvivorCarryoverTest.h"
#include "../testing/tests/ValidateShopFreezePurchaseTest.h"
#include "../testing/tests/ValidateShopFreezeSwapTest.h"
#include "../testing/tests/ValidateShopFreezeTest.h"
#include "../testing/tests/ValidateShopFreezeUnfreezeTest.h"
#include "../testing/tests/ValidateShopFunctionalityTest.h"
#include "../testing/tests/ValidateShopNavigationTest.h"
#include "../testing/tests/ValidateShopPurchaseExactGoldTest.h"
#include "../testing/tests/ValidateShopPurchaseFullInventoryTest.h"
#include "../testing/tests/ValidateShopPurchaseInsufficientFundsTest.h"
#include "../testing/tests/ValidateShopPurchaseNoGoldTest.h"
#include "../testing/tests/ValidateShopPurchaseNonexistentItemTest.h"
#include "../testing/tests/ValidateShopPurchaseTest.h"
#include "../testing/tests/ValidateShopPurchaseWrongScreenTest.h"
#include "../testing/tests/ValidateThreeBattlesFlowTest.h"
#include "../testing/tests/ValidateToastSystemTest.h"
#include "../testing/tests/ValidateTriggerOrderingTest.h"
#include "../testing/tests/ValidateTestInputFrameworkTest.h"
#include "../testing/tests/ValidateTriggerSystemComponentsTest.h"
#include "../testing/tests/ValidateTriggerSystemTest.h"
#include "../testing/tests/ValidateUINavigationTest.h"

void TestSystem::register_test_cases() {
  // Force initialization of TestRegistry and all static test registrars
  // by accessing the registry first (ensures singleton is created)
  auto &registry = TestRegistry::get();

  // Force static initialization of test registrars by referencing them
  // The TEST() macro creates static TestRegistrar_##name variables that
  // register themselves in their constructors. We need to ensure these
  // have been initialized. Accessing the registry should trigger any
  // initialization that needs to happen.

  // Get the test list after registry is initialized
  auto test_list = registry.list_tests();

  // Debug: log all registered tests to help diagnose registration issues
  log_info("TEST REGISTRY: Found {} registered tests", test_list.size());
  for (const auto &name : test_list) {
    log_info("TEST REGISTRY:   - {}", name);
  }
  log_info("TEST REGISTRY: Looking for test: {}", test_name);

  // If test not found, it might be due to static initialization order.
  // Try accessing the registry again and re-check.
  if (std::find(test_list.begin(), test_list.end(), test_name) ==
      test_list.end()) {
    // Force a second check - sometimes static initialization happens lazily
    log_info("TEST REGISTRY: Test not found in first pass, re-checking...");
    test_list = registry.list_tests();
    log_info("TEST REGISTRY: Second pass found {} tests", test_list.size());
  }

  bool found_in_new_registry = false;
  for (const auto &name : test_list) {
    if (name == test_name) {
      found_in_new_registry = true;
      log_info("TEST REGISTRY: Found test '{}' in registry", test_name);
      break;
    }
  }

  if (found_in_new_registry) {
    // New TEST() macro pattern - run test with TestApp in main game loop
    // context Tests use yield/resume pattern for one-time execution
    std::string test_name_copy = test_name; // Copy for lambda capture

    test_function = [test_name_copy]() {
      TestApp *&test_app_ptr = g_test_apps[test_name_copy];
      std::function<void()> &test_continuation =
          g_test_continuations[test_name_copy];

      if (test_app_ptr == nullptr) {
        log_info("TEST SYSTEM: Starting new TEST() macro test: {}",
                 test_name_copy);
        test_app_ptr = new TestApp();
        test_app_ptr->game_launched = false;
        test_continuation = nullptr;
      }

      try {
        // If we have a continuation, resume from where we yielded
        if (test_continuation) {
          test_continuation();
          test_continuation = nullptr;
          // Continuation calls run_test, which will skip completed operations
          // and continue from next line - check if test completed after
          // continuation
        } else {
          // Call the test - it will yield if it needs to wait
          bool test_done =
              TestRegistry::get().run_test(test_name_copy, *test_app_ptr);
          if (test_done) {
            // Test actually completed
            log_info("TEST PASSED: {}", test_name_copy);
            delete test_app_ptr;
            g_test_apps.erase(test_name_copy);
            g_test_continuations.erase(test_name_copy);
            exit(0);
          }
        }

        // Check if test yielded (stored a continuation)
        if (test_app_ptr && test_app_ptr->yield_continuation) {
          test_continuation = test_app_ptr->yield_continuation;
          test_app_ptr->yield_continuation = nullptr;
          // Test yielded - will resume when wait completes
          return;
        }

        // Check if we're still waiting for something
        if (test_app_ptr &&
            test_app_ptr->wait_state.type != TestApp::WaitState::None) {
          // Still waiting - don't complete yet
          return;
        }

        // No wait state and no yield - check if test completed
        // If test_in_progress is false, run_test() completed the test
        if (test_app_ptr && !test_app_ptr->test_in_progress) {
          // Test completed - run_test() set test_in_progress = false
          log_info("TEST PASSED: {}", test_name_copy);
          delete test_app_ptr;
          g_test_apps.erase(test_name_copy);
          g_test_continuations.erase(test_name_copy);
          exit(0);
        }

        // Test is still in progress but no waits/yields - this shouldn't happen
        // but if it does, the test will continue on the next frame
        return;
      } catch (const std::exception &e) {
        std::string error_msg = e.what();
        // Check if this is a yield exception
        if (error_msg == "TEST_YIELD") {
          // Test yielded - continuation is stored in
          // test_app_ptr->yield_continuation
          if (test_app_ptr && test_app_ptr->yield_continuation) {
            test_continuation = test_app_ptr->yield_continuation;
            test_app_ptr->yield_continuation = nullptr;
          }
          return;
        }
        // Real error, fail the test
        log_error("TEST FAILED: {} - {}", test_name_copy, e.what());
        if (test_app_ptr) {
          delete test_app_ptr;
        }
        g_test_apps.erase(test_name_copy);
        g_test_continuations.erase(test_name_copy);
        exit(1);
      }
    };

    // No validation function - we check waits in once() and resume continuation
    validation_function = nullptr;

    return;
  }

  // If we get here, the test is not registered via the new TEST() macro
  // This might happen if:
  // 1. Static initialization hasn't completed yet (try again)
  // 2. Test name doesn't match exactly
  // 3. Test file wasn't included
  log_error("Test not found: {}", test_name);
  log_error("Available tests in registry ({} total):",
            registry.list_tests().size());
  for (const auto &name : registry.list_tests()) {
    log_error("  - {}", name);
  }
  log_error("This might be a static initialization order issue.");
  log_error("Please verify the test file is included and the test name matches "
            "exactly.");

  // Set up a fallback: try to find the test again on next frame
  // This allows static initialization to complete if it hasn't yet
  std::string test_name_copy = test_name;
  test_function = [test_name_copy]() {
    auto &registry = TestRegistry::get();
    auto test_list = registry.list_tests();
    if (std::find(test_list.begin(), test_list.end(), test_name_copy) !=
        test_list.end()) {
      log_info("TEST REGISTRY: Test '{}' found on retry!", test_name_copy);
      // Test was registered, set up normal test execution
      static TestApp *test_app_ptr = nullptr;
      if (test_app_ptr == nullptr) {
        test_app_ptr = new TestApp();
        test_app_ptr->game_launched = false;
      }
      bool completed =
          TestRegistry::get().run_test(test_name_copy, *test_app_ptr);
      if (completed) {
        log_info("TEST PASSED: {}", test_name_copy);
        delete test_app_ptr;
        test_app_ptr = nullptr;
        exit(0);
      }
    } else {
      log_error("TEST REGISTRY: Test '{}' still not found on retry",
                test_name_copy);
      exit(1);
    }
  };
}
