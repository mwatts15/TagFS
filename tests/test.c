#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "test.h"

CU_ErrorCode add_tests (CU_suite_desc* suites, CU_test_desc* tests)
{
    for (int i = 0; suites[i].test_case != NULL; i++)
    {
        CU_suite_desc sd = suites[i];
        *sd.test_case = CU_add_suite(sd.test_case_name, sd.setup, sd.teardown);

        if (*sd.test_case == NULL)
        {
            CU_cleanup_registry();
            return CU_get_error();
        }
    }

    for (int i = 0; tests[i].test_case != NULL; i++)
    {
        CU_test_desc td = tests[i];
        if (CU_add_test(*td.test_case, td.test_name, td.f) == NULL)
        {
            CU_cleanup_registry();
            return CU_get_error();
        }
    }
    return 0;
}

CU_ErrorCode do_tests (CU_suite_desc* suites, CU_test_desc* tests)
{
    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();
    /* add a suite to the registry */
    add_tests(suites, tests);

    /* Run all tests using the basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    printf("\n");
    CU_basic_show_failures(CU_get_failure_list());
    printf("\n\n");

    CU_cleanup_registry();
    return 0;
}
