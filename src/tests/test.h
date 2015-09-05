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
    CU_InitializeFunc setupSuite;
    CU_CleanupFunc teardownSuite;
    CU_SetUpFunc setup;
    CU_TearDownFunc teardown;
} CU_suite_desc;

#define TEST(test_suite, test_fn) {&test_suite, #test_fn, test_fn}
#define SUITE(test_suite) {&test_suite, #test_suite, NULL, NULL}
#define SUITE_FULL(test_suite, __setup, __teardown) {&test_suite, #test_suite, NULL, NULL, __setup, __teardown}

#define CU_ASSERT_EQUAL_STRLEN_STACK 256

#undef CU_ASSERT_EQUAL
#undef CU_ASSERT_EQUAL_FATAL
#define CU_ASSERT_EQUAL_BASE(actual, expected, fatal) \
{ \
    char str[CU_ASSERT_EQUAL_STRLEN_STACK]; \
    unsigned _actual_value = (unsigned)(actual); \
    unsigned _expected_value = (unsigned)(expected); \
    snprintf(str, CU_ASSERT_EQUAL_STRLEN_STACK, \
            "CU_ASSERT_EQUAL(%s '0x%x', %s '0x%x')", #actual, _actual_value, #expected, (unsigned)_expected_value); \
    CU_assertImplementation(((_actual_value) == (_expected_value)), \
            __LINE__, str, __FILE__, "", fatal); \
}
#define CU_ASSERT_EQUAL(actual, expected) CU_ASSERT_EQUAL_BASE(actual, expected, CU_FALSE)
#define CU_ASSERT_EQUAL_FATAL(actual, expected) CU_ASSERT_EQUAL_BASE(actual, expected, CU_TRUE)

#undef CU_ASSERT_NOT_EQUAL
#define CU_ASSERT_NOT_EQUAL(actual, expected) \
{ \
    char str[CU_ASSERT_EQUAL_STRLEN_STACK]; \
    unsigned _actual_value = (unsigned)(actual); \
    unsigned _expected_value = (unsigned)(expected); \
    snprintf(str, CU_ASSERT_EQUAL_STRLEN_STACK, \
            "CU_ASSERT_NOT_EQUAL(%s '0x%x', %s '0x%x')", #actual, _actual_value, #expected, (unsigned)_expected_value); \
    CU_assertImplementation(((_actual_value) != (_expected_value)), \
            __LINE__, str, __FILE__, "", CU_FALSE); \
}

#undef CU_ASSERT_PTR_EQUAL
#define CU_ASSERT_PTR_EQUAL(actual, expected) \
{ \
    char str[CU_ASSERT_EQUAL_STRLEN_STACK]; \
    void* _actual_value = (void*)(actual); \
    void* _expected_value = (void*)(expected); \
    snprintf(str, CU_ASSERT_EQUAL_STRLEN_STACK, \
            "CU_ASSERT_PTR_EQUAL(%s '%p', %s '%p')", #actual, _actual_value, #expected, _expected_value); \
    CU_assertImplementation(((_actual_value) == (_expected_value)), \
            __LINE__, str, __FILE__, "", CU_FALSE); \
}

#undef CU_ASSERT_STRING_EQUAL
#define CU_ASSERT_STRING_EQUAL(actual, expected) \
{ \
    char str[CU_ASSERT_EQUAL_STRLEN_STACK]; \
    const char *_actual_value = (const char*)(actual); \
    const char *_expected_value = (const char*)(expected); \
    snprintf(str, CU_ASSERT_EQUAL_STRLEN_STACK, \
            "CU_ASSERT_STRING_EQUAL\nexpected: %s\n    with value \"%s\"\ngot: %s\n    with value \"%s\"", #actual, _actual_value, #expected, _expected_value); \
    CU_assertImplementation(!(strcmp(_actual_value, _expected_value)), \
            __LINE__, str, __FILE__, "", CU_FALSE); \
}

#undef CU_ASSERT_NOT_NULL
#define CU_ASSERT_NOT_NULL(actual) CU_ASSERT_PTR_NOT_EQUAL((actual), NULL)

#undef CU_ASSERT_NULL
#define CU_ASSERT_NULL(actual) CU_ASSERT_PTR_EQUAL((gconstpointer)(actual), NULL)

#undef CU_ASSERT_REGEX_MATCHES
#define CU_ASSERT_REGEX_MATCHES(actual, regex) \
{ \
    char _str[CU_ASSERT_EQUAL_STRLEN_STACK]; \
    const char* _actual_value = (char*)(actual); \
    const char* _regex_string = (char*)(regex); \
    GError *_gerror = NULL; \
    GRegex *_gr = g_regex_new(_regex_string, 0, 0, &_gerror); \
    gboolean _matches = g_regex_match(_gr, _actual_value, 0, NULL); \
    g_regex_unref(_gr); \
    snprintf(_str, CU_ASSERT_EQUAL_STRLEN_STACK, \
            "%s should match %s", _actual_value, _regex_string); \
    CU_assertImplementation(_matches, \
            __LINE__, _str, __FILE__, "", CU_FALSE); \
}

#undef CU_ASSERT_GREATER_THAN
#undef CU_ASSERT_GREATER_THAN_FATAL
#define CU_ASSERT_GREATER_THAN_BASE(actual, expected, fatal) \
{ \
    char str[CU_ASSERT_EQUAL_STRLEN_STACK]; \
    unsigned _actual_value = (unsigned)(actual); \
    unsigned _expected_value = (unsigned)(expected); \
    snprintf(str, CU_ASSERT_EQUAL_STRLEN_STACK, \
            "%s '0x%x' > %s '0x%x'", #actual, _actual_value, #expected, (unsigned)_expected_value); \
    CU_assertImplementation(((_actual_value) > (_expected_value)), \
            __LINE__, str, __FILE__, "", fatal); \
}

#define CU_ASSERT_GREATER_THAN(actual, expected)\
    CU_ASSERT_GREATER_THAN_BASE(actual, expected, CU_FALSE)

#define CU_ASSERT_GREATER_THAN_FATAL(actual, expected)\
    CU_ASSERT_GREATER_THAN_BASE(actual, expected, CU_TRUE)

#undef CU_ASSERT_LESS_THAN
#undef CU_ASSERT_LESS_THAN_FATAL
#define CU_ASSERT_LESS_THAN_BASE(actual, expected, fatal) \
{ \
    char str[CU_ASSERT_EQUAL_STRLEN_STACK]; \
    unsigned _actual_value = (unsigned)(actual); \
    unsigned _expected_value = (unsigned)(expected); \
    snprintf(str, CU_ASSERT_EQUAL_STRLEN_STACK, \
            "%s '0x%x' < %s '0x%x'", #actual, _actual_value, #expected, (unsigned)_expected_value); \
    CU_assertImplementation(((_actual_value) < (_expected_value)), \
            __LINE__, str, __FILE__, "", fatal); \
}

#define CU_ASSERT_LESS_THAN(actual, expected)\
    CU_ASSERT_LESS_THAN_BASE(actual, expected, CU_FALSE)

#define CU_ASSERT_LESS_THAN_FATAL(actual, expected)\
    CU_ASSERT_LESS_THAN_BASE(actual, expected, CU_TRUE)

int do_tests (CU_suite_desc* suites, CU_test_desc* tests);

#endif /* TEST_H */

