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
CU_ErrorCode do_tests (CU_suite_desc* suites, CU_test_desc* tests);

#endif /* TEST_H */

