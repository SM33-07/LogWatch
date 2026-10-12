#include "test_utils.h"

static void test_harness_assert_true(void) {
    ASSERT(1 == 1, "Trivial boolean truth assertion");
}

static void test_harness_assert_eq(void) {
    ASSERT_EQ(42, 42, "Integer equality assertion");
    ASSERT_EQ(0, 0, "Zero equality assertion");
    ASSERT_EQ(-10, -10, "Negative integer equality assertion");
}

static void test_harness_assert_str_eq(void) {
    ASSERT_STR_EQ("SYNTHETIC", "SYNTHETIC", "String exact match assertion");
    ASSERT_STR_EQ("/api/snippets", "/api/snippets", "Route string equality assertion");
}

int main(void) {
    TEST_BEGIN("Infrastructure Smoke Test");
    RUN_TEST(test_harness_assert_true);
    RUN_TEST(test_harness_assert_eq);
    RUN_TEST(test_harness_assert_str_eq);
    return TEST_REPORT();
}
