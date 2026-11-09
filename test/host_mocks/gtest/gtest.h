#pragma once

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace testing {

struct TestEntry {
  const char* suite;
  const char* name;
  std::function<void()> fn;
};

inline std::vector<TestEntry>& registry() {
  static std::vector<TestEntry> tests;
  return tests;
}

inline int& currentTestFailures() {
  static int failures = 0;
  return failures;
}

struct Registrar {
  Registrar(const char* suite, const char* name, std::function<void()> fn) {
    registry().push_back({suite, name, std::move(fn)});
  }
};

struct FailMessage {
  const char* file;
  int line;
  std::ostringstream ss;

  FailMessage(const char* f, int l) : file(f), line(l) {
    currentTestFailures()++;
  }
  ~FailMessage() {
    std::cerr << file << ":" << line << ": Failure: " << ss.str() << "\n";
  }
  template <typename T>
  FailMessage& operator<<(const T& val) {
    ss << val;
    return *this;
  }
};

struct Voidify {
  void operator&(const FailMessage&) const {}
};

namespace detail {

template <typename A, typename B>
bool eq(const A& a, const B& b) {
  if constexpr (std::is_enum_v<A> && std::is_enum_v<B>) {
    return a == b;
  } else if constexpr (std::is_integral_v<A> && std::is_integral_v<B>) {
    if constexpr (std::is_signed_v<A> == std::is_signed_v<B>) {
      using Common = std::common_type_t<A, B>;
      return static_cast<Common>(a) == static_cast<Common>(b);
    } else if constexpr (std::is_signed_v<A>) {
      return a >= 0 && static_cast<std::make_unsigned_t<A>>(a) == b;
    } else {
      return b >= 0 && a == static_cast<std::make_unsigned_t<B>>(b);
    }
  } else {
    return a == b;
  }
}

template <typename A, typename B>
bool ge(const A& a, const B& b) {
  if constexpr (std::is_integral_v<A> && std::is_integral_v<B>) {
    if constexpr (std::is_signed_v<A> == std::is_signed_v<B>) {
      using Common = std::common_type_t<A, B>;
      return static_cast<Common>(a) >= static_cast<Common>(b);
    } else if constexpr (std::is_signed_v<A>) {
      return a >= 0 && static_cast<std::make_unsigned_t<A>>(a) >= b;
    } else {
      return b < 0 || a >= static_cast<std::make_unsigned_t<B>>(b);
    }
  } else {
    return a >= b;
  }
}

}  // namespace detail

inline void InitGoogleTest(int* argc, char** argv) {
  (void)argc;
  (void)argv;
}

inline int RunAllTests() {
  int total = 0;
  int failed_tests = 0;
  for (const auto& t : registry()) {
    total++;
    currentTestFailures() = 0;
    std::cout << "[ RUN      ] " << t.suite << "." << t.name << "\n";
    t.fn();
    if (currentTestFailures() == 0) {
      std::cout << "[       OK ] " << t.suite << "." << t.name << "\n";
    } else {
      failed_tests++;
      std::cout << "[  FAILED  ] " << t.suite << "." << t.name << "\n";
    }
  }
  std::cout << "[==========] " << total << " test(s) ran.\n";
  if (failed_tests == 0) {
    std::cout << "[  PASSED  ] " << total << " test(s).\n";
    return 0;
  }
  std::cerr << "[  FAILED  ] " << failed_tests << " test(s).\n";
  return 1;
}

}  // namespace testing

#define RUN_ALL_TESTS() ::testing::RunAllTests()

#define TEST(suite, name)                                                     \
  static void suite##_##name##_TestBody();                                    \
  static ::testing::Registrar suite##_##name##_Reg(                           \
      #suite, #name, &suite##_##name##_TestBody);                             \
  static void suite##_##name##_TestBody()

#define EXPECT_TRUE(cond)                                                     \
  do {                                                                        \
    if (!(cond)) {                                                            \
      ::testing::FailMessage(__FILE__, __LINE__)                              \
          << "Expected true: " << #cond;                                      \
    }                                                                         \
  } while (0)

#define EXPECT_FALSE(cond)                                                    \
  do {                                                                        \
    if (cond) {                                                               \
      ::testing::FailMessage(__FILE__, __LINE__)                              \
          << "Expected false: " << #cond;                                     \
    }                                                                         \
  } while (0)

#define ASSERT_TRUE(cond)                                                     \
  do {                                                                        \
    if (!(cond)) {                                                            \
      ::testing::FailMessage(__FILE__, __LINE__)                              \
          << "Assertion failed: " << #cond;                                   \
      return;                                                                 \
    }                                                                         \
  } while (0)

#define EXPECT_EQ(a, b)                                                       \
  do {                                                                        \
    if (!::testing::detail::eq((a), (b))) {                                   \
      ::testing::FailMessage(__FILE__, __LINE__)                              \
          << "Expected equality: (" << #a << ") == (" << #b << ")";           \
    }                                                                         \
  } while (0)

#define EXPECT_GE(a, b)                                                       \
  do {                                                                        \
    if (!::testing::detail::ge((a), (b))) {                                   \
      ::testing::FailMessage(__FILE__, __LINE__)                              \
          << "Expected (" << #a << ") >= (" << #b << ")";                     \
    }                                                                         \
  } while (0)

#define EXPECT_NEAR(a, b, tol)                                                \
  do {                                                                        \
    const double diff_ = std::fabs(static_cast<double>(a) -                   \
                                   static_cast<double>(b));                   \
    if (diff_ > static_cast<double>(tol)) {                                   \
      ::testing::FailMessage(__FILE__, __LINE__)                              \
          << "Expected |" << #a << " - " << #b << "| <= " << #tol             \
          << " (diff=" << diff_ << ")";                                       \
    }                                                                         \
  } while (0)

#define FAIL()                                                                \
  return ::testing::Voidify() & ::testing::FailMessage(__FILE__, __LINE__)
