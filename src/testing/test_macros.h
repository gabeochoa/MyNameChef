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

    try {
      log_info("TEST_DEBUG: run_test({}) - calling test function, wait_state.type={}", 
               name, static_cast<int>(app.wait_state.type));
      it->second.test_fn(app);

      log_info("TEST_DEBUG: run_test({}) - test function returned, wait_state.type={}", 
               name, static_cast<int>(app.wait_state.type));

      // If test completed but we're still waiting for something, return false
      // to indicate test needs to continue on next frame
      if (app.wait_state.type != TestApp::WaitState::None) {
        log_info("TEST_DEBUG: run_test({}) - returning false (wait_state.type={})", 
                 name, static_cast<int>(app.wait_state.type));
        return false; // Need to continue waiting
      }

      log_info("TEST PASSED: {}", name);
      return true;
    } catch (const std::exception &e) {
      // Check if this is a "continue" exception (non-blocking wait)
      std::string error_msg = e.what();
      log_info("TEST_DEBUG: run_test({}) - exception: {}", name, error_msg);
      if (error_msg == "WAIT_FOR_UI_CONTINUE" ||
          error_msg == "WAIT_FOR_SCREEN_CONTINUE" ||
          error_msg == "WAIT_FOR_FRAME_DELAY_CONTINUE") {
        log_info("TEST_DEBUG: run_test({}) - continue exception, returning false", name);
        return false; // Test needs to continue on next frame
      }

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
