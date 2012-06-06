#ifndef TEST_UTIL_H
#define TEST_UTIL_H
#include "types.h"
/* 
   These must be used
   so that the results of
   the tests can be parsed
 */
void print_result(char *test_name, gboolean test_result);
void res_info (result_t *r);
#endif /* TEST_UTIL_H */
