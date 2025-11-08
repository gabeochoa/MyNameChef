#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include <afterhours/ah.h>

#include "../game_state_manager.h"
#include "../log.h"
#include "../render_backend.h"
#include "../rl.h"
#include "../testing/test_app.h"

// Forward declarations for static maps in TestSystem.cpp
extern std::unordered_map<std::string, TestApp *> g_test_apps;
extern std::unordered_map<std::string, std::function<void()>>
    g_test_continuations;

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
          // Check for integration, opponent_match, or checksum_match (all are
          // integration tests)
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
      // Check wait conditions and resume test when waits complete
      auto it = g_test_apps.find(test_name);
      if (it != g_test_apps.end() && it->second != nullptr) {
        TestApp *test_app = it->second;

        // Check if we're waiting for something
        if (test_app->wait_state.type != TestApp::WaitState::None) {
          validation_attempts++;
          // Skip accumulating time on first validation frame (dt may be huge
          // due to initialization)
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

          bool wait_complete = test_app->check_wait_conditions();
          if (wait_complete &&
              test_app->wait_state.type == TestApp::WaitState::None) {
            // Wait completed - resume test via continuation if available,
            // otherwise re-run
            auto cont_it = g_test_continuations.find(test_name);
            if (cont_it != g_test_continuations.end() && cont_it->second) {
              // Resume from continuation
              if (test_function) {
                test_function();
              }
            } else {
              // No continuation - re-run test function (completed operations
              // will be skipped)
              if (test_function) {
                test_function();
              }
            }
          } else {
            // Still waiting
            if (validation_attempts % 100 == 0) {
              log_info(
                  "TEST VALIDATION CHECKING: {} - Attempt {} ({}s) - Still "
                  "waiting...",
                  test_name, validation_attempts, validation_elapsed_time);
            }
          }
        } else {
          // No wait state - test should have completed or be waiting for next
          // operation Don't re-run here - let the test function complete
          // naturally
        }
      } else if (!test_completed) {
        test_completed = true;
        log_info("TEST COMPLETED: {} - Test finished", test_name);
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

    // If test completed immediately (no waits), exit
    if (test_completed) {
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
