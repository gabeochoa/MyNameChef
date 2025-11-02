#include "TestSystem.h"

// Include rl.h first to ensure Vector2Type is defined as raylib::Vector2
// before any afterhours UI headers are included (fixes Vector2Type conversion
// issues)
#include "../rl.h" // NOLINT - required for Vector2Type fix

#include "../testing/test_macros.h"
// Include all test files so TEST() macro registrations run
#include "../testing/tests/GotoBattleTest.h"
#include "../testing/tests/PlayNavigatesToShopTest.h"
#include "../testing/tests/ValidateBattleResultsTest.h"
#include "../testing/tests/ValidateCombatSystemTest.h"
#include "../testing/tests/ValidateDebugDishTest.h"
#include "../testing/tests/ValidateDishOrderingTest.h"
#include "../testing/tests/ValidateDishSystemTest.h"
#include "../testing/tests/ValidateEffectSystemTest.h"
#include "../testing/tests/ValidateFullBattleFlowTest.h"
#include "../testing/tests/ValidateFullGameFlowTest.h"
#include "../testing/tests/ValidateMainMenuTest.h"
#include "../testing/tests/ValidateReplayPausePlayTest.h"
#include "../testing/tests/ValidateShopFunctionalityTest.h"
#include "../testing/tests/ValidateShopNavigationTest.h"
#include "../testing/tests/ValidateShopPurchaseExactGoldTest.h"
#include "../testing/tests/ValidateShopPurchaseFullInventoryTest.h"
#include "../testing/tests/ValidateShopPurchaseInsufficientFundsTest.h"
#include "../testing/tests/ValidateShopPurchaseNoGoldTest.h"
#include "../testing/tests/ValidateShopPurchaseNonexistentItemTest.h"
#include "../testing/tests/ValidateShopPurchaseTest.h"
#include "../testing/tests/ValidateShopPurchaseWrongScreenTest.h"
#include "../testing/tests/ValidateTriggerSystemComponentsTest.h"
#include "../testing/tests/ValidateTriggerSystemTest.h"
#include "../testing/tests/ValidateUINavigationTest.h"

void TestSystem::register_test_cases() {
  // First check if test is registered via new TEST() macro
  auto &registry = TestRegistry::get();
  auto test_list = registry.list_tests();
  bool found_in_new_registry = false;
  for (const auto &name : test_list) {
    if (name == test_name) {
      found_in_new_registry = true;
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
  // This should not happen if all tests have been migrated
  log_error("Test not found: {}", test_name);
}
