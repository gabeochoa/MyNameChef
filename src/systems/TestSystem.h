#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include <afterhours/ah.h>

#include "../game_state_manager.h"
#include "../log.h"
#include "../render_backend.h"
#include "../rl.h"

struct TestSystem : afterhours::System<> {
  std::string test_name;
  bool initialized = false;
  bool validation_completed = false;
  bool test_completed = false;
  int validation_attempts = 0;
  float validation_start_time = 0.0f;
  float validation_elapsed_time = 0.0f;
  bool first_validation_frame = true;
  // Use longer timeout in visible mode (slower due to rendering)
  const float kValidationTimeout;
  std::function<void()> test_function;
  std::function<bool()> validation_function;

  explicit TestSystem(std::string name)
      : test_name(name), kValidationTimeout([test_name_copy = name]() {
          // Integration tests need much longer timeouts for full battles
          // Check for integration, opponent_match, or checksum_match (all are integration tests)
          bool is_integration =
              test_name_copy.find("integration") != std::string::npos ||
              test_name_copy.find("opponent_match") != std::string::npos ||
              test_name_copy.find("checksum_match") != std::string::npos;
          if (is_integration) {
            return render_backend::is_headless_mode ? 30.0f : 120.0f;
          }
          // Server failure tests use forced checks and fast intervals
          // Should complete quickly with forced check mechanism
          bool is_server_failure =
              test_name_copy.find("server_failure") != std::string::npos;
          if (is_server_failure) {
            float timeout = render_backend::is_headless_mode ? 10.0f : 10.0f;
            return timeout;
          }
          return render_backend::is_headless_mode ? 1.0f : 10.0f;
        }()) {
    log_info("TEST SYSTEM CREATED: {} - TestSystem instantiated (timeout: {}s)",
             test_name, kValidationTimeout);
    register_test_cases();
  }

  virtual bool should_run(float) override { return true; }

  virtual void once(float dt) override {
    if (initialized) {
      // Run validation on subsequent frames
      if (!validation_completed && validation_function) {
        validation_attempts++;
        // Skip accumulating time on first validation frame (dt may be huge due
        // to initialization)
        if (first_validation_frame) {
          first_validation_frame = false;
        } else {
          // Cap dt to reasonable frame time (16ms at 60fps) to avoid huge
          // spikes
          float clamped_dt = std::min(dt, 0.1f);
          // Divide by timing speed scale to track real wall-clock time
          // instead of game time (dt is already multiplied by the scale)
          float real_dt = clamped_dt / render_backend::timing_speed_scale;
          validation_elapsed_time += real_dt;
        }

        if (validation_elapsed_time >= kValidationTimeout) {
          log_error("TEST VALIDATION TIMEOUT: {} - Validation exceeded {}s "
                    "timeout ({} attempts)",
                    test_name, kValidationTimeout, validation_attempts);
          exit(1);
        }

        bool validation_result = validation_function();
        if (validation_result) {
          validation_completed = true;
          test_completed = true;
          log_info("TEST VALIDATION PASSED: {} - Validation successful after "
                   "{} attempts ({}s)",
                   test_name, validation_attempts, validation_elapsed_time);
          exit(0);
        } else {
          if (validation_attempts % 100 == 0) {
            log_info("TEST VALIDATION CHECKING: {} - Attempt {} ({}s) - Still "
                     "waiting for validation...",
                     test_name, validation_attempts, validation_elapsed_time);
          }
        }
      } else if (!test_completed) {
        test_completed = true;
        log_info("TEST COMPLETED: {} - No validation function, test finished",
                 test_name);
        exit(0);
      }
      return;
    }

    initialized = true;
    validation_start_time = 0.0f;
    validation_elapsed_time = 0.0f;
    log_info("TEST EXECUTING: {} - Running test logic", test_name);

    if (test_function) {
      test_function();
    }

    if (!validation_function) {
      test_completed = true;
      log_info("TEST COMPLETED: {} - No validation function, test finished "
               "immediately",
               test_name);
      exit(0);
    }
  }

  virtual void for_each(afterhours::Entity &, float) override {}

  virtual void after(float) override {
    GameStateManager::get().update_screen();
  }

private:
  void register_test_cases();

public:
  // Public method to force test registration (used by --list-tests)
  static void force_test_registration() {
    TestSystem dummy("__force_registration__");
    dummy.register_test_cases();
  }
};
