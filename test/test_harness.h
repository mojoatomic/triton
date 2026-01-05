/**
 * RC Submarine Controller - Unit Test Harness
 *
 * A minimal test framework for embedded C code.
 * Tests run on the host machine with mocked hardware.
 *
 * Usage:
 *   #include "test_harness.h"
 *
 *   TEST(test_name) {
 *       ASSERT_EQ(1, 1);
 *       ASSERT_TRUE(condition);
 *   }
 *
 *   TEST_MAIN()
 */

#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================
// Test Framework
// ============================================================

static int _test_pass_count = 0;
static int _test_fail_count = 0;
static int _assert_count = 0;
static const char* _current_test = NULL;

#define TEST(name) \
    static void test_##name(void); \
    static void _run_test_##name(void) { \
        _current_test = #name; \
        printf("  [TEST] %s\n", #name); \
        test_##name(); \
    } \
    static void test_##name(void)

// Internal assertion helper
#define _ASSERT_IMPL(cond, msg, ...) do { \
    _assert_count++; \
    if (!(cond)) { \
        printf("    FAIL: %s:%d: " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
        _test_fail_count++; \
        return; \
    } \
} while(0)

// ============================================================
// Assertions
// ============================================================

#define ASSERT_TRUE(cond) \
    _ASSERT_IMPL((cond), "Expected true, got false: %s", #cond)

#define ASSERT_FALSE(cond) \
    _ASSERT_IMPL(!(cond), "Expected false, got true: %s", #cond)

#define ASSERT_EQ(expected, actual) \
    _ASSERT_IMPL((expected) == (actual), \
        "Expected %d, got %d", (int)(expected), (int)(actual))

#define ASSERT_NEQ(expected, actual) \
    _ASSERT_IMPL((expected) != (actual), \
        "Expected not %d, got %d", (int)(expected), (int)(actual))

#define ASSERT_LT(a, b) \
    _ASSERT_IMPL((a) < (b), "Expected %d < %d", (int)(a), (int)(b))

#define ASSERT_LE(a, b) \
    _ASSERT_IMPL((a) <= (b), "Expected %d <= %d", (int)(a), (int)(b))

#define ASSERT_GT(a, b) \
    _ASSERT_IMPL((a) > (b), "Expected %d > %d", (int)(a), (int)(b))

#define ASSERT_GE(a, b) \
    _ASSERT_IMPL((a) >= (b), "Expected %d >= %d", (int)(a), (int)(b))

#define ASSERT_FLOAT_EQ(expected, actual, epsilon) \
    _ASSERT_IMPL(fabs((expected) - (actual)) < (epsilon), \
        "Expected %f, got %f (epsilon=%f)", \
        (double)(expected), (double)(actual), (double)(epsilon))

#define ASSERT_STR_EQ(expected, actual) \
    _ASSERT_IMPL(strcmp((expected), (actual)) == 0, \
        "Expected \"%s\", got \"%s\"", (expected), (actual))

#define ASSERT_NULL(ptr) \
    _ASSERT_IMPL((ptr) == NULL, "Expected NULL, got %p", (void*)(ptr))

#define ASSERT_NOT_NULL(ptr) \
    _ASSERT_IMPL((ptr) != NULL, "Expected non-NULL, got NULL")

#define ASSERT_MEM_EQ(expected, actual, size) \
    _ASSERT_IMPL(memcmp((expected), (actual), (size)) == 0, \
        "Memory mismatch at offset %zu", \
        _find_mem_diff((expected), (actual), (size)))

// Helper to find first differing byte
static inline size_t _find_mem_diff(const void* a, const void* b, size_t size) {
    const unsigned char* pa = (const unsigned char*)a;
    const unsigned char* pb = (const unsigned char*)b;
    for (size_t i = 0; i < size; i++) {
        if (pa[i] != pb[i]) return i;
    }
    return size;
}

// ============================================================
// Test Runner
// ============================================================

// Test registration (simple array-based)
typedef void (*test_func_t)(void);
typedef struct {
    const char* name;
    test_func_t func;
} test_entry_t;

#define MAX_TESTS 100
static test_entry_t _test_registry[MAX_TESTS];
static int _test_count = 0;

#define REGISTER_TEST(name) \
    static void _run_test_##name(void); \
    __attribute__((constructor)) static void _register_##name(void) { \
        if (_test_count < MAX_TESTS) { \
            _test_registry[_test_count].name = #name; \
            _test_registry[_test_count].func = _run_test_##name; \
            _test_count++; \
        } \
    }

// Alternative: Manual test list
#define RUN_TEST(name) do { \
    int _before = _test_fail_count; \
    _run_test_##name(); \
    if (_test_fail_count == _before) _test_pass_count++; \
} while(0)

#define TEST_MAIN() \
    int main(int argc, char** argv) { \
        (void)argc; (void)argv; \
        printf("\n"); \
        printf("════════════════════════════════════════════════════════════\n"); \
        printf("  Running tests: %s\n", __FILE__); \
        printf("════════════════════════════════════════════════════════════\n"); \
        printf("\n");

#define TEST_MAIN_END() \
        printf("\n"); \
        printf("════════════════════════════════════════════════════════════\n"); \
        printf("  Results: %d passed, %d failed (%d assertions)\n", \
               _test_pass_count, _test_fail_count, _assert_count); \
        printf("════════════════════════════════════════════════════════════\n"); \
        return _test_fail_count > 0 ? 1 : 0; \
    }

// ============================================================
// Mock Helpers
// ============================================================

// Call counting for mocks
#define MOCK_CALL_COUNT(name) _mock_##name##_call_count
#define MOCK_RESET(name) _mock_##name##_call_count = 0
#define MOCK_CALLED(name) (MOCK_CALL_COUNT(name) > 0)
#define MOCK_CALL_COUNT_EQ(name, n) (MOCK_CALL_COUNT(name) == (n))

// Declare a mock call counter
#define DECLARE_MOCK_COUNT(name) static int _mock_##name##_call_count = 0
#define INCREMENT_MOCK_COUNT(name) _mock_##name##_call_count++

#endif // TEST_HARNESS_H
