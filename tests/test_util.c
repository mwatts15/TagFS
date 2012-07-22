#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include "test_util.h"
#include "types.h"
#include "util.h"
#include "log.h"

void print_result(char *test_name, gboolean test_result)
{
    printf("%s : TEST %s\n", test_name, test_result?"PASSED":"FAILED");
}

void open_test_log ()
{
    log_open("test.log", 1);
}
