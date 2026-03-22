#ifndef TEST_MACROS_H
#define TEST_MACROS_H

#include <stdio.h>
#include <string.h>

static int test_count = 0;
static int pass_count = 0;

#define TEST_CASE(name) \
    printf("--- Test Case: %s ---\n", name);

#define ASSERT(condition) \
    do { \
        test_count++; \
        if (condition) { \
            pass_count++; \
            printf("  [PASS] %s\n", #condition); \
        } else { \
            printf("  [FAIL] %s at %s:%d\n", #condition, __FILE__, __LINE__); \
        } \
    } while (0)

#define ASSERT_STR_EQ(s1, s2) \
    do { \
        test_count++; \
        if (strcmp(s1, s2) == 0) { \
            pass_count++; \
            printf("  [PASS] \"%s\" == \"%s\"\n", s1, s2); \
        } else { \
            printf("  [FAIL] \"%s\" != \"%s\" at %s:%d\n", s1, s2, __FILE__, __LINE__); \
        } \
    } while (0)

#define TEST_SUMMARY() \
    printf("\n--- Summary ---\n"); \
    printf("Total tests: %d\n", test_count); \
    printf("Passed: %d\n", pass_count); \
    printf("Failed: %d\n", test_count - pass_count); \
    if (test_count != pass_count) { \
        return 1; \
    } \
    return 0;

#endif // TEST_MACROS_H
