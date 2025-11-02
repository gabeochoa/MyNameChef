#include "TestSystem.h"

// Include rl.h first to ensure Vector2Type is defined as raylib::Vector2
// before any afterhours UI headers are included (fixes Vector2Type conversion
// issues)
#include "../rl.h" // NOLINT - required for Vector2Type fix

#include "../testing/test_macros.h"
#include "../testing/tests/GotoBattleTest.h"
#include "../testing/tests/PlayNavigatesToShopTest.h"
#include "../testing/tests/ValidateBattleResultsTest.h"
#include "../testing/tests/ValidateCombatSystemTest.h"
#include "../testing/tests/ValidateDebugDishTest.h"
#include "../testing/tests/ValidateDishSystemTest.h"
#include "../testing/tests/ValidateEffectSystemTest.h"
#include "../testing/tests/ValidateFullGameFlowTest.h"
#include "../testing/tests/ValidateMainMenuTest.h"
#include "../testing/tests/ValidateShopNavigationTest.h"
#include "../testing/tests/ValidateTriggerSystemTest.h"
#include "../testing/tests/ValidateUINavigationTest.h"

std::unordered_map<std::string, std::function<void()>>
    TestSystem::test_registry;

void TestSystem::register_test_cases() {
  // Register test cases using individual test files
  test_registry["goto_battle"] = []() { GotoBattleTest::execute(); };

  // Register validation functions
  if (test_name == "validate_combat_system") {
    validation_function = []() {
      return ValidateCombatSystemTest::validate_battle_screen();
    };
  } else if (test_name == "validate_battle_results") {
    validation_function = []() {
      return ValidateBattleResultsTest::validate_results_screen();
    };
  } else if (test_name == "validate_ui_navigation") {
    validation_function = []() {
      return ValidateUINavigationTest::validate_ui_elements();
    };
  } else if (test_name == "validate_full_game_flow") {
    validation_function = []() {
      return ValidateFullGameFlowTest::validate_complete_flow();
    };
  } else if (test_name == "validate_trigger_system") {
    validation_function = []() {
      return ValidateTriggerSystemTest::validate_trigger_events();
    };
  }

  test_registry["validate_trigger_system"] = []() {
    ValidateTriggerSystemTest::execute();
  };

  test_registry["validate_combat_system"] = []() {
    ValidateCombatSystemTest::execute();
  };

  test_registry["validate_dish_system"] = []() {
    ValidateDishSystemTest::execute();
  };

  test_registry["validate_debug_dish"] = []() {
    ValidateDebugDishTest::execute();
  };

  test_registry["validate_battle_results"] = []() {
    ValidateBattleResultsTest::execute();
  };

  test_registry["validate_ui_navigation"] = []() {
    ValidateUINavigationTest::execute();
  };

  test_registry["validate_full_game_flow"] = []() {
    ValidateFullGameFlowTest::execute();
  };

  test_registry["validate_trigger_system"] = []() {
    ValidateTriggerSystemTest::execute();
  };

  test_registry["validate_effect_system"] = []() {
    ValidateEffectSystemTest::execute();
  };

  // Focused single test to isolate salmon persistence investigation
  test_registry["validate_salmon_persistence"] = []() {
    ValidateEffectSystemTest::execute_salmon_persistence_only();
  };

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
        // Test not completed yet - will continue on next frame
      } catch (const std::exception &e) {
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
            log_error("TEST FAILED: {} - {}", test_name_copy, e.what());
            delete test_app_ptr;
            test_app_ptr = nullptr;
            return true; // Mark as complete (failed)
          }
        }
        return false; // Still waiting
      }

      return false;
    };

    return;
  }

  // Look up the test function in old registry
  auto it = test_registry.find(test_name);
  if (it != test_registry.end()) {
    test_function = it->second;
  }
}
