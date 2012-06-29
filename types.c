#include <stdlib.h>
#include <glib.h>

#include "types.h"
#include "util.h"
#include "set_ops.h"

const char *type_strings[] = {
    "DICT",
    "LIST",
    "INT",
    "STRING"
};

void tagdb_value_set_type(tagdb_value_t *v, int type)
{
    v->type = type;
}

int tagdb_value_get_type(tagdb_value_t *v)
{
    return v->type;
}

char *hash_to_string (GHashTable *hsh)
{
    if (hsh == NULL)
        return "";
    GString *accu = g_string_new("");
    HL(hsh, it, k, v)
        char *this = tagdb_value_to_str((tagdb_value_t*) v);
        char *escaped = g_strescape(this, "");

        g_string_append_printf(accu, "%d\t", TO_I(k));
        g_string_append_printf(accu, "%s\t", escaped);
        
        g_free(this);
        g_free(escaped);
    HL_END;
    char *res = g_strdup(accu->str);
    g_string_free(accu, TRUE);
    return res;
}

char *list_to_string (GList *l)
{
    GString *accu = g_string_new("");
    LL(l, it)
        char *this = tagdb_value_to_str((tagdb_value_t*) it->data);
        char *escaped = g_strescape(this, "");

        g_string_append_printf(accu, "%s\t", this);

        g_free(escaped);
        g_free(this);
    LL_END(it);
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
        return -1;
    if (a != NULL && b == NULL)
        return 1;
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

    int lt = tagdb_value_get_type(lhs);
    int rt = tagdb_value_get_type(rhs);

    if (lt - rt)
        return lt - rt;

    switch (lt)
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
    if (tagdb_value_get_type(lhs) != tagdb_value_get_type(rhs))
        return FALSE;
    switch (tagdb_value_get_type(lhs))
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
    tagdb_value_set_type(res, type);
    return res;
}

char *tagdb_value_to_str (tagdb_value_t *value)
{
    switch (tagdb_value_get_type(value))
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
    tagdb_value_set_type(res,type);

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
    switch (tagdb_value_get_type(v))
    {
        case tagdb_dict_t:
            data = v->data.d;
            break;
        case tagdb_int_t:
            data = TO_P(v->data.i);
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
    return encapsulate(tagdb_value_get_type(v), data);
}
