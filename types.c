#include <stdlib.h>
#include "types.h"
#include "util.h"
#include "set_ops.h"

const char *type_strings[] = {
    "DICT",
    "LIST",
    "INT",
    "STRING"
};

/*
   there are at least four kinds of hashes we deal with:
       1. (file_ids::INT => GHashTable*)
       2. (file_ids::INT => tagdb_value_t*)
       3. (tag_codes::INT => GHashTable*)
       4. (tag_codes::INT => tagdb_value_t*)
   TODO: Each must be handled differently
 */
char *hash_to_string (GHashTable *hsh)
{
    if (hsh == NULL)
        return "";
    GString *accu = g_string_new("");
    gpointer k, v;
    GHashTableIter it;
    //tagdb_value_t *value = NULL;
    g_hash_loop(hsh, it, k, v)
    {
        g_string_append_printf(accu, "%d ", TO_I(k));
    }
    char *res = g_strdup(accu->str);
    g_string_free(accu, TRUE);
    return res;
}

/* TODO: lists should contain only tagdb_value_t */ 
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

gboolean g_list_cmp (GList *a, GList *b)
{
    int res = 1;
    while (a != NULL && b != NULL)
    {
        res = tagdb_value_cmp(a->data, b->data);
        if (res != 0)
            return res;
    }
    // sort shorter lists before longer
    if (a == NULL && b != NULL)
        return 1;
    if (a != NULL && b == NULL)
        return -1;
    return res;
}

gboolean g_list_equal (GList *a, GList *b)
{
    while (a != NULL)
    {
        if (b == NULL || !tagdb_value_equals(a->data, b->data))
            return FALSE;
        a = a->next;
        b = b->next;
    }
    return TRUE;
}

int tagdb_value_cmp (tagdb_value_t *lhs, tagdb_value_t *rhs)
{
    if (lhs == rhs)
        return 0;
    // {lhs->type == rhs->type}
    switch (lhs->type)
    {
        case (tagdb_dict_t):
            return set_cmp_s(lhs->data.d, rhs->data.d);
        case (tagdb_list_t):
            return g_list_cmp(lhs->data.l, rhs->data.l);
        case (tagdb_int_t):
            return (lhs->data.i - rhs->data.i);
        case (tagdb_str_t):
            return (g_strcmp0(lhs->data.s, rhs->data.s));
        default:
            return 1;
    }
    return 1;
}

gboolean tagdb_value_equals (tagdb_value_t *lhs, tagdb_value_t *rhs)
{
    if (lhs == rhs)
        return TRUE;
    if (lhs->type != rhs->type)
        return FALSE;
    switch (lhs->type)
    {
        case (tagdb_dict_t):
            return set_equal_s(lhs->data.d, rhs->data.d);
        case (tagdb_list_t):
            return g_list_equal(lhs->data.l, rhs->data.l);
        case (tagdb_int_t):
            return (lhs->data.i == rhs->data.i);
        case (tagdb_str_t):
            return (g_strcmp0(lhs->data.s, rhs->data.s) == 0);
        default:
            return FALSE;
    }
    return FALSE;
}

tagdb_value_t *tagdb_str_to_value (int type, char *data)
{
    tagdb_value_t *res = malloc(sizeof(tagdb_value_t));
    switch (type)
    {
        case (tagdb_dict_t):
        case (tagdb_list_t):
            res->data.b = NULL; // I don't want to deal with this yet -_-
            break;
        case (tagdb_int_t):
            res->data.i = atoi(data);
            break;
        case (tagdb_str_t):
            res->data.s = g_strdup(data);
            break;
        default:
            res->data.b = (gpointer) data;
            break;
    }
    res->type = type;
    return res;
}

char *tagdb_value_to_str (tagdb_value_t *value)
{
    switch (value->type)
    {
        case (tagdb_dict_t):
            return hash_to_string(value->data.d);
        case (tagdb_list_t):
            return list_to_string(value->data.l);
        case (tagdb_int_t):
            return g_strdup_printf("%d", value->data.i);
        case (tagdb_str_t):
            //printf("VALUE %s\n", value->s);
            return g_strdup(value->data.s);
        case (tagdb_err_t):
            if (value->data.s != NULL)
                return g_strdup_printf("ERROR: %s", value->data.s);
            else
                return g_strdup_printf("ERROR in tagdb_result_t"); 
        default:
            return g_strdup("BINDATA\n");
    }
}

// -> (type, *object*)
// encapsualtes the object in a result type
// with info about the type based on type
// we pass the database because encapsulation may make further
// queries to acquire data that the user expects
result_t *encapsulate (int type, gpointer data)
{
    result_t *res = g_malloc(sizeof(result_t));
    res->type = type;
    switch (type)
    {
        case tagdb_dict_t:
            res->data.d = data;
            break;
        case tagdb_int_t:
            res->data.i = GPOINTER_TO_INT(data);
            break;
        case tagdb_str_t:
            res->data.s = data;
            break;
        case tagdb_err_t:
            if (data == NULL)
                res->data.s = NULL;
            else
                res->data.s = data;
            break;
        default:
            res->data.b = data;
    }
    return res;
}

void query_destroy (query_t *q)
{
    //g_strfreev(q->argv);
    g_free(q);
}

void result_destroy (result_t *r)
{
    g_free(r);
}

tagdb_value_t *default_value (int type)
{
    switch (type)
    {
        case tagdb_dict_t:
            return encapsulate(type, g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL));
            break;
        case tagdb_int_t:
            return encapsulate(type, 0);
            break;
        case tagdb_str_t:
            return encapsulate(type, "");
            break;
        case tagdb_err_t:
            return encapsulate(type, "ERROR");
        default:
            return encapsulate(type, NULL);
            break;
    }
}

tagdb_value_t *copy_value (tagdb_value_t *v)
{
    gpointer data = NULL;
    switch (v->type)
    {
        case tagdb_dict_t:
            data = v->data.d;
            break;
        case tagdb_int_t:
            data = v->data.i;
            break;
        case tagdb_str_t:
            data = v->data.s;
            break;
        case tagdb_err_t:
            if (data != NULL)
                data = v->data.s;
            break;
        default:
            data = v->data.b;
    }
    return encapsulate(v->type, data);
}
