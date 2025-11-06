#pragma once

#include "../log.h"
#include "battle_simulator.h"
#include <exception>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace server {
namespace test {

struct TestCase {
  std::string name;
  std::function<void()> test_fn;
  std::string file;
  int line;

  TestCase(const std::string &n, std::function<void()> fn, const std::string &f,
           int l)
      : name(n), test_fn(std::move(fn)), file(f), line(l) {}
};

class TestRegistry {
public:
  static TestRegistry &get() {
    static TestRegistry instance;
    return instance;
  }

  void register_test(const std::string &name, std::function<void()> fn,
                     const std::string &file, int line) {
    tests.push_back(TestCase(name, fn, file, line));
  }

  bool run_all_tests() {
    int passed = 0;
    int failed = 0;

    for (const auto &test : tests) {
      try {
        test.test_fn();
        // Clean up entities after each test to prevent state conflicts
        server::BattleSimulator::cleanup_test_entities();
        passed++;
        log_info("TEST PASSED: {}", test.name);
      } catch (const std::exception &e) {
        // Still cleanup even if test failed
        server::BattleSimulator::cleanup_test_entities();
        failed++;
        log_error("TEST FAILED: {} - {}", test.name, e.what());
        log_error("  Location: {}:{}", test.file, test.line);
      } catch (...) {
        // Still cleanup even if test failed
        server::BattleSimulator::cleanup_test_entities();
        failed++;
        log_error("TEST FAILED: {} - Unknown exception", test.name);
        log_error("  Location: {}:{}", test.file, test.line);
      }
    }

    log_info("Test Results: {} passed, {} failed, {} total", passed, failed,
             tests.size());
    return failed == 0;
  }

  std::vector<std::string> list_tests() const {
    std::vector<std::string> names;
    for (const auto &test : tests) {
      names.push_back(test.name);
    }
    return names;
  }

private:
  std::vector<TestCase> tests;
};

#define SERVER_TEST(name)                                                      \
  void test_##name();                                                          \
  struct TestRegistrar_##name {                                                \
    TestRegistrar_##name() {                                                   \
      server::test::TestRegistry::get().register_test(#name, test_##name,      \
                                                      __FILE__, __LINE__);     \
    }                                                                          \
  } test_registrar_##name;                                                     \
  void test_##name()

#define ASSERT_TRUE(condition)                                                 \
  do {                                                                         \
    if (!(condition)) {                                                        \
      throw std::runtime_error("Assertion failed: " #condition);               \
    }                                                                          \
  } while (0)

#define ASSERT_FALSE(condition)                                                \
  do {                                                                         \
    if (condition) {                                                           \
      throw std::runtime_error("Assertion failed: !" #condition);              \
    }                                                                          \
  } while (0)

template <typename T>
void assert_eq_impl(const T &expected, const T &actual, const char *file,
                    int line) {
  if (expected != actual) {
    std::stringstream ss;
    ss << "Assertion failed at " << file << ":" << line
       << ": expected == actual";
    throw std::runtime_error(ss.str());
  }
}

template <typename T>
void assert_ne_impl(const T &expected, const T &actual, const char *file,
                    int line) {
  if (expected == actual) {
    std::stringstream ss;
    ss << "Assertion failed at " << file << ":" << line
       << ": expected != actual";
    throw std::runtime_error(ss.str());
  }
}

#define ASSERT_EQ(expected, actual)                                            \
  server::test::assert_eq_impl(expected, actual, __FILE__, __LINE__)
#define ASSERT_NE(expected, actual)                                            \
  server::test::assert_ne_impl(expected, actual, __FILE__, __LINE__)

#define ASSERT_STREQ(expected, actual)                                         \
  do {                                                                         \
    if (std::string(expected) != std::string(actual)) {                        \
      throw std::runtime_error("Assertion failed: expected \"" +               \
                               std::string(expected) + "\", got \"" +          \
                               std::string(actual) + "\"");                    \
    }                                                                          \
  } while (0)

} // namespace test
} // namespace server
