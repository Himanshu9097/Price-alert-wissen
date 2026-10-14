#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <sstream>
#include <exception>

namespace test_harness {

struct TestCase {
    std::string name;
    std::function<void()> func;
};

inline std::vector<TestCase>& get_registry() {
    static std::vector<TestCase> registry;
    return registry;
}

struct AutoRegister {
    AutoRegister(const std::string& name, std::function<void()> func) {
        get_registry().push_back({name, std::move(func)});
    }
};

class TestFailure : public std::exception {
public:
    TestFailure(std::string msg, const char* file, int line)
        : msg_(std::move(msg)), file_(file), line_(line) {
        std::ostringstream oss;
        oss << file_ << ":" << line_ << ": " << msg_;
        full_msg_ = oss.str();
    }

    const char* what() const noexcept override {
        return full_msg_.c_str();
    }

private:
    std::string msg_;
    const char* file_;
    int line_;
    std::string full_msg_;
};

#define TEST_CASE(name) \
    void name(); \
    static ::test_harness::AutoRegister reg_##name(#name, name); \
    void name()

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            throw ::test_harness::TestFailure("Assertion failed: " #cond " is false", __FILE__, __LINE__); \
        } \
    } while (0)

#define ASSERT_FALSE(cond) \
    do { \
        if (cond) { \
            throw ::test_harness::TestFailure("Assertion failed: " #cond " is true", __FILE__, __LINE__); \
        } \
    } while (0)

#define ASSERT_EQ(actual, expected) \
    do { \
        auto&& _act = (actual); \
        auto&& _exp = (expected); \
        if (!(_act == _exp)) { \
            std::ostringstream _oss; \
            _oss << "Assertion failed: (" #actual " == " #expected ") with values [" \
                 << _act << " != " << _exp << "]"; \
            throw ::test_harness::TestFailure(_oss.str(), __FILE__, __LINE__); \
        } \
    } while (0)

#define ASSERT_NE(actual, expected) \
    do { \
        auto&& _act = (actual); \
        auto&& _exp = (expected); \
        if (_act == _exp) { \
            std::ostringstream _oss; \
            _oss << "Assertion failed: (" #actual " != " #expected ") both evaluate to [" \
                 << _act << "]"; \
            throw ::test_harness::TestFailure(_oss.str(), __FILE__, __LINE__); \
        } \
    } while (0)

#define ASSERT_THROWS(expr, ExceptionType) \
    do { \
        bool _threw = false; \
        try { \
            expr; \
        } catch (const ExceptionType&) { \
            _threw = true; \
        } catch (...) { \
            throw ::test_harness::TestFailure("Expression " #expr " threw unexpected exception type", __FILE__, __LINE__); \
        } \
        if (!_threw) { \
            throw ::test_harness::TestFailure("Expression " #expr " expected to throw " #ExceptionType " but did not", __FILE__, __LINE__); \
        } \
    } while (0)

#define ASSERT_NO_THROW(expr) \
    do { \
        try { \
            expr; \
        } catch (const std::exception& _e) { \
            throw ::test_harness::TestFailure(std::string("Expression " #expr " threw exception: ") + _e.what(), __FILE__, __LINE__); \
        } catch (...) { \
            throw ::test_harness::TestFailure("Expression " #expr " threw unknown exception", __FILE__, __LINE__); \
        } \
    } while (0)

inline int run_all_tests() {
    auto& tests = get_registry();
    int passed = 0;
    int failed = 0;

    std::cout << "==================================================\n";
    std::cout << "Running " << tests.size() << " automated unit/integration tests...\n";
    std::cout << "==================================================\n";

    for (const auto& test : tests) {
        try {
            test.func();
            std::cout << "  [PASS] " << test.name << "\n";
            ++passed;
        } catch (const TestFailure& tf) {
            std::cout << "  [FAIL] " << test.name << "\n";
            std::cout << "         " << tf.what() << "\n";
            ++failed;
        } catch (const std::exception& e) {
            std::cout << "  [FAIL] " << test.name << " (unexpected std::exception: " << e.what() << ")\n";
            ++failed;
        } catch (...) {
            std::cout << "  [FAIL] " << test.name << " (unknown exception)\n";
            ++failed;
        }
    }

    std::cout << "==================================================\n";
    std::cout << "Test Summary: " << passed << " passed, " << failed << " failed (Total " << tests.size() << ")\n";
    std::cout << "==================================================\n";

    return (failed == 0) ? 0 : 1;
}

} // namespace test_harness
