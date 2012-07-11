#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include "test_util.h"
#include "types.h"
#include "util.h"

void print_result(char *test_name, gboolean test_result)
{
    printf("%s : TEST %s\n", test_name, test_result?"PASSED":"FAILED");
}

void res_info (result_t *r)
{
    if (r == NULL)
    {
        return;
    }
    printf("result info:\n");
    printf("\ttype: %s\n", type_strings[r->type]);
    printf("\tdata: ");
    switch (r->type)
    {
        case tagdb_dict_t:
            print_hash(r->data.d);
            break;
        case tagdb_list_t:
            print_list(r->data.l, (ToString)tagdb_value_to_str);
            break;
        case tagdb_int_t:
            printf("%zd\n", r->data.i);
            break;
        case tagdb_str_t:
            printf("%s\n", r->data.s);
            break;
        case tagdb_err_t:
            printf("%s\n", r->data.s);
            break;
        default:
            printf("%p\n", r->data.b);
    }
}

