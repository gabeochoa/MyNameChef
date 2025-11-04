#include "TestSystem.h"

// Include rl.h first to ensure Vector2Type is defined as raylib::Vector2
// before any afterhours UI headers are included (fixes Vector2Type conversion
// issues)
#include "../rl.h" // NOLINT - required for Vector2Type fix

#include "../testing/test_macros.h"
#include <algorithm>

// Include all test files so TEST() macro registrations run
// Static TestRegistrar_##name objects will register themselves during static
// initialization
#include "../testing/tests/GotoBattleTest.h"
#include "../testing/tests/PlayNavigatesToShopTest.h"
#include "../testing/tests/ValidateBattleResultsTest.h"
#include "../testing/tests/ValidateCombatSystemTest.h"
#include "../testing/tests/ValidateDebugDishTest.h"
#include "../testing/tests/ValidateDishMergingTest.h"
#include "../testing/tests/ValidateDishOrderingTest.h"
#include "../testing/tests/ValidateDishSellingTest.h"
#include "../testing/tests/ValidateDishSystemTest.h"
#include "../testing/tests/ValidateEffectSystemTest.h"
#include "../testing/tests/ValidateFullBattleFlowTest.h"
#include "../testing/tests/ValidateFullGameFlowTest.h"
#include "../testing/tests/ValidateMainMenuTest.h"
#include "../testing/tests/ValidateReplayPausePlayTest.h"
#include "../testing/tests/ValidateRerollCostTest.h"
#include "../testing/tests/ValidateSeededRngTest.h"
#include "../testing/tests/ValidateShopFreezeTest.h"
#include "../testing/tests/ValidateShopFunctionalityTest.h"
#include "../testing/tests/ValidateShopNavigationTest.h"
#include "../testing/tests/ValidateShopPurchaseExactGoldTest.h"
#include "../testing/tests/ValidateShopPurchaseFullInventoryTest.h"
#include "../testing/tests/ValidateShopPurchaseInsufficientFundsTest.h"
#include "../testing/tests/ValidateShopPurchaseNoGoldTest.h"
#include "../testing/tests/ValidateShopPurchaseNonexistentItemTest.h"
#include "../testing/tests/ValidateShopPurchaseTest.h"
#include "../testing/tests/ValidateShopPurchaseWrongScreenTest.h"
#include "../testing/tests/ValidateTriggerOrderingTest.h"
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
    // context Tests can use non-blocking waits that continue across frames
    std::string test_name_copy = test_name; // Copy for lambda capture
    static TestApp *test_app_ptr = nullptr; // Persist across frames

    test_function = [test_name_copy]() {
      if (test_app_ptr == nullptr) {
        log_info("TEST SYSTEM: Starting new TEST() macro test: {}",
                 test_name_copy);
        test_app_ptr = new TestApp();
        // Reset launch flag for new test instance
        test_app_ptr->game_launched = false;
      }

      try {
        bool completed =
            TestRegistry::get().run_test(test_name_copy, *test_app_ptr);

        if (completed) {
          log_info("TEST PASSED: {}", test_name_copy);
          delete test_app_ptr;
          test_app_ptr = nullptr;
          exit(0);
        }
        // Test not completed yet - will continue on next frame via validation
        // function
      } catch (const std::exception &e) {
        // Check if this is a "continue" exception (non-blocking wait)
        std::string error_msg = e.what();
        if (error_msg == "WAIT_FOR_UI_CONTINUE" ||
            error_msg == "WAIT_FOR_SCREEN_CONTINUE" ||
            error_msg == "WAIT_FOR_FRAME_DELAY_CONTINUE") {
          // Wait was set up, validation function will handle continuation
          // Don't delete test_app_ptr here - it's needed for validation
          return;
        }
        log_error("TEST FAILED: {} - {}", test_name_copy, e.what());
        delete test_app_ptr;
        test_app_ptr = nullptr;
        exit(1);
      }
    };

    // Validation function checks wait conditions each frame
    validation_function = [test_name_copy]() {
      if (test_app_ptr == nullptr) {
        return false;
      }

      // Check if we're waiting for something
      if (test_app_ptr->wait_state.type != TestApp::WaitState::None) {
        bool wait_complete = test_app_ptr->check_wait_conditions();
        if (wait_complete &&
            test_app_ptr->wait_state.type == TestApp::WaitState::None) {
          // Wait completed, try to continue test
          try {
            bool test_complete =
                TestRegistry::get().run_test(test_name_copy, *test_app_ptr);
            if (test_complete) {
              log_info("TEST PASSED: {}", test_name_copy);
              delete test_app_ptr;
              test_app_ptr = nullptr;
              return true;
            }
          } catch (const std::exception &e) {
            // Check if this is a "continue" exception (non-blocking wait)
            std::string error_msg = e.what();
            if (error_msg == "WAIT_FOR_UI_CONTINUE" ||
                error_msg == "WAIT_FOR_SCREEN_CONTINUE" ||
                error_msg == "WAIT_FOR_FRAME_DELAY_CONTINUE") {
              // Wait was set up again, continue checking
              return false;
            }
            log_error("TEST FAILED: {} - {}", test_name_copy, e.what());
            delete test_app_ptr;
            test_app_ptr = nullptr;
            exit(1);
          }
        }
        return false; // Still waiting
      }

      // No wait state - test should have completed. Try running it one more
      // time to see if it's done
      try {
        bool test_complete =
            TestRegistry::get().run_test(test_name_copy, *test_app_ptr);
        if (test_complete) {
          log_info("TEST PASSED: {}", test_name_copy);
          delete test_app_ptr;
          test_app_ptr = nullptr;
          return true;
        }
        // Test not complete - check if wait state was set up by run_test()
        // (e.g., by wait_for_frames() throwing WAIT_FOR_FRAME_DELAY_CONTINUE)
        if (test_app_ptr->wait_state.type != TestApp::WaitState::None) {
          // Wait state was set up, return false and let wait handling logic
          // deal with it on the next validation check
          return false;
        }
        // No wait state and test not complete - might be stuck, return false to
        // keep checking (will retry on next frame)
        return false;
      } catch (const std::exception &e) {
        // Check if this is a "continue" exception (non-blocking wait)
        std::string error_msg = e.what();
        if (error_msg == "WAIT_FOR_UI_CONTINUE" ||
            error_msg == "WAIT_FOR_SCREEN_CONTINUE" ||
            error_msg == "WAIT_FOR_FRAME_DELAY_CONTINUE") {
          // Wait was set up, return false to continue checking
          return false;
        }
        // Real error, fail the test
        log_error("TEST FAILED: {} - {}", test_name_copy, e.what());
        delete test_app_ptr;
        test_app_ptr = nullptr;
        exit(1);
      }
    };

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
