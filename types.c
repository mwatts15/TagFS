#include <stdlib.h>
#include "types.h"

union tagdb_value *tagdb_str_to_value (int type, char *data)
{
    union tagdb_value *res = malloc(sizeof(union tagdb_value));
    switch (type)
    {
        case (tagdb_dict_t):
        case (tagdb_list_t):
            res->b = NULL; // I don't want to deal with this yet -_-
            break;
        case (tagdb_int_t):
            res->i = atoi(data);
            break;
        case (tagdb_str_t):
            res->s = data;
            break;
        default:
            res->b = (gpointer) data;
            break;
    }
    return res;
}

char *tagdb_value_to_str (int type, union tagdb_value *value)
{
    switch (type)
    {
        case (tagdb_dict_t):
        case (tagdb_list_t):
            return g_strdup("NIL");
        case (tagdb_int_t):
            return g_strdup_printf("%d", value->i);
        case (tagdb_str_t):
            //printf("VALUE %s\n", value->s);
            return g_strdup(value->s);
        case (tagdb_err_t):
            return g_strdup_printf("ERROR: %s", value->s);
        default:
            return g_strdup("BINDATA");
    }
}
