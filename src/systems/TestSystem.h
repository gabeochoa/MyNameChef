#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include <afterhours/ah.h>

#include "../game_state_manager.h"
#include "../log.h"

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
      test_function();
    }

    // Check if validation function exists and mark completion
    if (!validation_function) {
      test_completed = true;
      log_info("TEST COMPLETED: {} - No validation function, test finished "
               "immediately",
               test_name);
      exit(0); // Exit with success
    }
  }

  virtual void for_each(afterhours::Entity &, float) override {}

  virtual void after(float) override {
    GameStateManager::get().update_screen();
  }

private:
  void register_test_cases();
};
