#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include <afterhours/ah.h>

#include "../game_state_manager.h"
#include "../log.h"
#include "../testing/tests/GotoBattleTest.h"
#include "../testing/tests/PlayNavigatesToShopTest.h"
#include "../testing/tests/ValidateBattleResultsTest.h"
#include "../testing/tests/ValidateCombatSystemTest.h"
#include "../testing/tests/ValidateDishSystemTest.h"
#include "../testing/tests/ValidateFullGameFlowTest.h"
#include "../testing/tests/ValidateMainMenuTest.h"
#include "../testing/tests/ValidateShopFunctionalityTest.h"
#include "../testing/tests/ValidateShopNavigationTest.h"
#include "../testing/tests/ValidateTriggerSystemTest.h"
#include "../testing/tests/ValidateUINavigationTest.h"

struct TestSystem : afterhours::System<> {
  std::string test_name;
  bool initialized = false;
  bool validation_completed = false;
  bool test_completed = false;
  int validation_attempts = 0;
  std::function<void()> test_function;
  std::function<bool()> validation_function;

  explicit TestSystem(std::string name) : test_name(std::move(name)) {
    log_info("TEST SYSTEM CREATED: {} - TestSystem instantiated", test_name);
    register_test_cases();
  }

  virtual bool should_run(float) override { return true; }

  virtual void once(float) override {
    if (initialized) {
      // Run validation on subsequent frames
      if (!validation_completed && validation_function) {
        validation_attempts++;
        bool validation_result = validation_function();
        if (validation_result) {
          validation_completed = true;
          test_completed = true;
          log_info("TEST VALIDATION PASSED: {} - Validation successful after "
                   "{} attempts",
                   test_name, validation_attempts);
          exit(0); // Exit with success
        } else {
          // Log every 10 attempts to avoid spam
          if (validation_attempts % 10 == 0) {
            log_info("TEST VALIDATION CHECKING: {} - Attempt {} - Still "
                     "waiting for validation...",
                     test_name, validation_attempts);
          }
        }
      } else if (!test_completed) {
        // Test has no validation function, mark as completed
        test_completed = true;
        log_info("TEST COMPLETED: {} - No validation function, test finished",
                 test_name);
        exit(0); // Exit with success
      }
      return;
    }

    initialized = true;
    log_info("TEST EXECUTING: {} - Running test logic", test_name);

    if (test_function) {
      log_info("TEST SYSTEM DEBUG: Calling test_function");
      test_function();
      log_info("TEST SYSTEM DEBUG: test_function completed");
    }

    // Check if validation function exists and mark completion
    if (!validation_function) {
      log_info("TEST SYSTEM DEBUG: No validation function for test: {}",
               test_name);
      test_completed = true;
      log_info("TEST COMPLETED: {} - No validation function, test finished "
               "immediately",
               test_name);
      exit(0); // Exit with success
    } else {
      log_info("TEST SYSTEM DEBUG: Validation function exists for test: {}",
               test_name);
    }
  }

  virtual void for_each(afterhours::Entity &, float) override {}

  virtual void after(float) override {
    GameStateManager::get().update_screen();
  }

private:
  void register_test_cases() {
    // Register test cases using individual test files
    test_registry["play_navigates_to_shop"] = []() {
      PlayNavigatesToShopTest::execute();
    };

    test_registry["goto_battle"] = []() { GotoBattleTest::execute(); };

    test_registry["validate_shop_navigation"] = []() {
      ValidateShopNavigationTest::execute();
    };

    // Register validation functions
    if (test_name == "validate_shop_navigation") {
      validation_function = []() {
        return ValidateShopNavigationTest::validate_shop_screen();
      };
    } else if (test_name == "validate_shop_functionality") {
      validation_function = []() {
        return ValidateShopFunctionalityTest::validate_shop_complete();
      };
    } else if (test_name == "validate_combat_system") {
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

    test_registry["validate_shop_functionality"] = []() {
      ValidateShopFunctionalityTest::execute();
    };

    test_registry["validate_combat_system"] = []() {
      ValidateCombatSystemTest::execute();
    };

    test_registry["validate_dish_system"] = []() {
      ValidateDishSystemTest::execute();
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

    // Look up the test function
    auto it = test_registry.find(test_name);
    if (it != test_registry.end()) {
      test_function = it->second;
    }
  }

  static std::unordered_map<std::string, std::function<void()>> test_registry;
};
