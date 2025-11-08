#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#if __cplusplus >= 202002L && __has_include(<source_location>)
#include <source_location>
#define TEST_USE_SOURCE_LOCATION
#endif

#include "../log.h"
#include "test_app.h"

struct TestCase {
  std::string name;
  std::function<void(TestApp &)> test_fn;
  std::string file;
  int line = 0;

  TestCase() = default;

  TestCase(const std::string &n, std::function<void(TestApp &)> fn,
           const std::string &f, int l)
      : name(n), test_fn(std::move(fn)), file(f), line(l) {}
};

class TestRegistry {
public:
  static TestRegistry &get() {
    static TestRegistry instance;
    return instance;
  }

  void register_test(const std::string &name, std::function<void(TestApp &)> fn,
                     const std::string &file, int line) {
    tests[name] = TestCase(name, fn, file, line);
  }

  bool run_test(const std::string &name, TestApp &app) {
    auto it = tests.find(name);
    if (it == tests.end()) {
      return false;
    }

    app.set_test_name(name);

    // Guard against recursive calls
    if (app.test_executing) {
      // Already executing - this is a recursive call, return false to indicate
      // not done
      return false;
    }
    app.test_executing = true;

    // Only set test_in_progress if we're not resuming (resuming means we're
    // continuing, not starting)
    if (!app.test_resuming) {
      app.test_in_progress = true;
    }
    app.test_resuming = false;

    try {
      it->second.test_fn(app);

      // Reset executing flag before checking yield/wait state
      app.test_executing = false;

      // If test yielded, it will have stored a continuation and thrown
      // TEST_YIELD Check if test yielded
      if (app.yield_continuation) {
        // Test yielded - will resume when wait completes
        return false;
      }

      // If test completed but we're still waiting for something, return false
      // to indicate test needs to continue on next frame
      if (app.wait_state.type != TestApp::WaitState::None) {
        return false; // Need to continue waiting
      }

      // Test actually completed - reset flags and return true
      app.test_in_progress = false;
      app.test_executing = false;
      log_info("TEST PASSED: {}", name);
      return true;
    } catch (const std::exception &e) {
      // Check if this is a yield exception
      std::string error_msg = e.what();
      if (error_msg == "TEST_YIELD") {
        // Test yielded - continuation is stored in app.yield_continuation
        // test_in_progress remains true - test will continue
        app.test_executing = false; // Reset executing flag
        return false; // Test needs to continue on next frame after wait
                      // completes
      }

      // Legacy continue exceptions (should not be used with yield/resume, but
      // handle for compatibility)
      if (error_msg == "WAIT_FOR_UI_CONTINUE" ||
          error_msg == "WAIT_FOR_SCREEN_CONTINUE" ||
          error_msg == "WAIT_FOR_FRAME_DELAY_CONTINUE") {
        return false; // Test needs to continue on next frame
      }

      app.test_executing = false; // Reset executing flag on error
      log_error("TEST FAILED: {} - {}", name, e.what());
      log_error("  Location: {}:{}", it->second.file, it->second.line);
      if (!app.failure_location.empty()) {
        log_error("  Failure at: {}", app.failure_location);
      }
      throw; // Re-throw to let TestSystem handle it - will exit(1), never retry
    }
  }

  std::vector<std::string> list_tests() const {
    std::vector<std::string> result;
    for (const auto &[name, test] : tests) {
      result.push_back(name);
    }
    return result;
  }

private:
  std::unordered_map<std::string, TestCase> tests;
};

#ifdef TEST_USE_SOURCE_LOCATION
#define TEST(test_name)                                                        \
  void test_##test_name(TestApp &app);                                         \
  struct TestRegistrar_##test_name {                                           \
    TestRegistrar_##test_name() {                                              \
      auto loc = std::source_location::current();                              \
      TestRegistry::get().register_test(#test_name, test_##test_name,          \
                                        loc.file_name(), loc.line());          \
    }                                                                          \
  };                                                                           \
  static TestRegistrar_##test_name test_registrar_##test_name;                 \
  void test_##test_name(TestApp &app)
#else
#define TEST(test_name)                                                        \
  void test_##test_name(TestApp &app);                                         \
  struct TestRegistrar_##test_name {                                           \
    TestRegistrar_##test_name() {                                              \
      TestRegistry::get().register_test(#test_name, test_##test_name,          \
                                        __FILE__, __LINE__);                   \
    }                                                                          \
  };                                                                           \
  static TestRegistrar_##test_name test_registrar_##test_name;                 \
  void test_##test_name(TestApp &app)
#endif
