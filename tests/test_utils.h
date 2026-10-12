#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*
 * Test statistics tracking
 */
static int g_test_assertions_run = 0;
static int g_test_assertions_passed = 0;
static int g_test_assertions_failed = 0;

static int g_test_cases_run = 0;
static int g_test_cases_passed = 0;
static int g_test_cases_failed = 0;
static bool g_current_test_failed = false;

#define TEST_BEGIN(suite_name) \
    do { \
        printf("\n==================================================\n"); \
        printf("  TEST SUITE: %s [SYNTHETIC]\n", suite_name); \
        printf("==================================================\n"); \
        g_test_assertions_run = 0; \
        g_test_assertions_passed = 0; \
        g_test_assertions_failed = 0; \
        g_test_cases_run = 0; \
        g_test_cases_passed = 0; \
        g_test_cases_failed = 0; \
    } while (0)

#define RUN_TEST(test_func) \
    do { \
        g_current_test_failed = false; \
        g_test_cases_run++; \
        printf("[RUN ] %s\n", #test_func); \
        test_func(); \
        if (g_current_test_failed) { \
            g_test_cases_failed++; \
            printf("[FAIL] %s\n", #test_func); \
        } else { \
            g_test_cases_passed++; \
            printf("[PASS] %s\n", #test_func); \
        } \
    } while (0)

#define ASSERT(condition, message) \
    do { \
        g_test_assertions_run++; \
        if (condition) { \
            g_test_assertions_passed++; \
        } else { \
            g_test_assertions_failed++; \
            g_current_test_failed = true; \
            fprintf(stderr, "  ASSERTION FAILED at %s:%d: %s\n", __FILE__, __LINE__, message); \
            fprintf(stderr, "    Condition: (%s)\n", #condition); \
        } \
    } while (0)

#define ASSERT_EQ(actual, expected, message) \
    do { \
        g_test_assertions_run++; \
        long long _act = (long long)(actual); \
        long long _exp = (long long)(expected); \
        if (_act == _exp) { \
            g_test_assertions_passed++; \
        } else { \
            g_test_assertions_failed++; \
            g_current_test_failed = true; \
            fprintf(stderr, "  ASSERTION FAILED at %s:%d: %s\n", __FILE__, __LINE__, message); \
            fprintf(stderr, "    Actual:   %lld\n", _act); \
            fprintf(stderr, "    Expected: %lld\n", _exp); \
        } \
    } while (0)

#define ASSERT_STR_EQ(actual, expected, message) \
    do { \
        g_test_assertions_run++; \
        const char *_act = (const char *)(actual); \
        const char *_exp = (const char *)(expected); \
        if (_act != NULL && _exp != NULL && strcmp(_act, _exp) == 0) { \
            g_test_assertions_passed++; \
        } else { \
            g_test_assertions_failed++; \
            g_current_test_failed = true; \
            fprintf(stderr, "  ASSERTION FAILED at %s:%d: %s\n", __FILE__, __LINE__, message); \
            fprintf(stderr, "    Actual:   \"%s\"\n", _act ? _act : "(null)"); \
            fprintf(stderr, "    Expected: \"%s\"\n", _exp ? _exp : "(null)"); \
        } \
    } while (0)

#define TEST_REPORT() \
    ( \
        printf("\n--------------------------------------------------\n"), \
        printf("Tests run: %d | Passed: %d | Failed: %d\n", \
               g_test_cases_run, g_test_cases_passed, g_test_cases_failed), \
        printf("Assertions run: %d | Passed: %d | Failed: %d\n", \
               g_test_assertions_run, g_test_assertions_passed, g_test_assertions_failed), \
        printf("==================================================\n"), \
        (g_test_cases_failed == 0 && g_test_assertions_failed == 0) ? 0 : 1 \
    )

#endif /* TEST_UTILS_H */
