#include <stdlib.h>
#include "types.h"
#include "util.h"

char *hash_to_string (GHashTable *hsh)
{
    GString *accu = g_string_new("");
    gpointer k, v;
    GHashTableIter it;
    g_hash_loop(hsh, it, k, v)
    {
        union tagdb_value *tvalue;
        g_string_append_printf(accu, "%d ", TO_I(k));
    }
    char *res = g_strdup(accu->str);
    g_string_free(accu, TRUE);
    return res;
}

char *list_to_string (GList *l)
{
    GString *accu = g_string_new("");
    while (l != NULL)
    {
        g_strdup_printf("%s ", (char*) l->data);
    }
    char *res = g_strdup(accu->str);
    g_string_free(accu, TRUE);
    return res;
}

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
            return hash_to_string(value->d);
        case (tagdb_list_t):
            return list_to_string(value->l);
        case (tagdb_int_t):
            return g_strdup_printf("%d", value->i);
        case (tagdb_str_t):
            //printf("VALUE %s\n", value->s);
            return g_strdup(value->s);
        case (tagdb_err_t):
            if (value->s != NULL)
                return g_strdup_printf("ERROR: %s", value->s);
            else
                return g_strdup_printf("ERROR in result_t", value->s); 
        default:
            return g_strdup("BINDATA\n");
    }
}
