#ifndef TEST_H
#define TEST_H

#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    CU_pSuite *test_case;
    const char *test_name;
    CU_TestFunc f;
} CU_test_desc;

typedef struct {
    CU_pSuite *test_case;
    const char *test_case_name;
    CU_InitializeFunc setup;
    CU_CleanupFunc teardown;
} CU_suite_desc;

#define TEST(test_suite, test_fn) {&test_suite, #test_fn, test_fn}
#define SUITE(test_suite) {&test_suite, #test_suite, NULL, NULL}

#define CU_ASSERT_EQUAL_STRLEN_STACK 256

#undef CU_ASSERT_EQUAL
#define CU_ASSERT_EQUAL(actual, expected) \
{ \
    char str[CU_ASSERT_EQUAL_STRLEN_STACK]; \
    snprintf(str, CU_ASSERT_EQUAL_STRLEN_STACK, \
            "CU_ASSERT_EQUAL(%s '0x%x', %s '0x%x')", #actual, (unsigned)actual, #expected, (unsigned)expected); \
    CU_assertImplementation(((actual) == (expected)), \
            __LINE__, str, __FILE__, "", CU_FALSE); \
}

#undef CU_ASSERT_STRING_EQUAL
#define CU_ASSERT_STRING_EQUAL(actual, expected) \
{ \
    char str[CU_ASSERT_EQUAL_STRLEN_STACK]; \
    snprintf(str, CU_ASSERT_EQUAL_STRLEN_STACK, \
            "CU_ASSERT_STRING_EQUAL\nexpected: %s\n    with value \"%s\"\ngot: %s\n    with value \"%s\"", #actual, actual, #expected, expected); \
    CU_assertImplementation(!(strcmp((const char*)(actual), (const char*)(expected))), \
            __LINE__, str, __FILE__, "", CU_FALSE); \
}

#undef CU_ASSERT_NOT_NULL
#define CU_ASSERT_NOT_NULL(actual) CU_ASSERT_PTR_NOT_EQUAL((actual), NULL)

#undef CU_ASSERT_NULL
#define CU_ASSERT_NULL(actual) CU_ASSERT_PTR_EQUAL((gconstpointer)(actual), NULL)
CU_ErrorCode do_tests (CU_suite_desc* suites, CU_test_desc* tests);

#endif /* TEST_H */

