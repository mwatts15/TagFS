#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include "test_util.h"
#include "types.h"
#include "util.h"

void print_result(char *test_name, char *output_file, char *correct_file)
{
    char *diffcmd = g_strdup_printf("diff %s %s", output_file, correct_file);
    printf("%s : TEST %s\n", test_name,
            (system(diffcmd) != 0)?"FAILED":"PASSED");
    g_free(diffcmd);
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
            print_list(stdout, r->data.l);
            break;
        case tagdb_int_t:
            printf("%d\n", r->data.i);
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

