#include "TestSystem.h"

// Include rl.h first to ensure Vector2Type is defined as raylib::Vector2
// before any afterhours UI headers are included
#include "../rl.h"

#include "../testing/tests/GotoBattleTest.h"
#include "../testing/tests/PlayNavigatesToShopTest.h"
#include "../testing/tests/ValidateBattleResultsTest.h"
#include "../testing/tests/ValidateCombatSystemTest.h"
#include "../testing/tests/ValidateDishSystemTest.h"
#include "../testing/tests/ValidateEffectSystemTest.h"
#include "../testing/tests/ValidateFullGameFlowTest.h"
#include "../testing/tests/ValidateMainMenuTest.h"
#include "../testing/tests/ValidateShopFunctionalityTest.h"
#include "../testing/tests/ValidateShopNavigationTest.h"
#include "../testing/tests/ValidateTriggerSystemTest.h"
#include "../testing/tests/ValidateUINavigationTest.h"

std::unordered_map<std::string, std::function<void()>>
    TestSystem::test_registry;

void TestSystem::register_test_cases() {
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

  test_registry["validate_effect_system"] = []() {
    ValidateEffectSystemTest::execute();
  };

  // Look up the test function
  auto it = test_registry.find(test_name);
  if (it != test_registry.end()) {
    test_function = it->second;
    log_info("TEST SYSTEM DEBUG: Found test function for: {}", test_name);
  } else {
    log_info("TEST SYSTEM DEBUG: No test function found for: {}", test_name);
  }
}
