#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "types.h"
#include "util.h"
#include "set_ops.h"
#include "stream.h"

#define append0(_g_string) g_string_append_c(_g_string, '\0')
const char *type_strings[] = {
    "D",
    "L",
    "I",
    "S",
    "B"
};

guint tagdb_value_hash (tagdb_value_t *v)
{
    switch (v->type)
    {
        case (tagdb_str_t):
            return g_str_hash(v->data.s);
        case (tagdb_int_t):
            return g_direct_hash(TO_64P(v->data.i));
        default:
            return 0;
    }
}

void tagdb_value_set_type(tagdb_value_t *v, int type)
{
    v->type = type;
}

int tagdb_value_get_type(tagdb_value_t *v)
{
    return v->type;
}

/* "I[8 bytes]" */

char *to_binstring_i (tagdb_value_t *v, size_t *size)
{
    GString *accu = g_string_new("I");
    char *s = int64_to_charp(v->data.i);
    g_string_append_len(accu, s, TAGDB_VALUE_INT_SIZE);
    g_free(s);
    *size = TAGDB_VALUE_INT_SIZE + 1;
    return g_string_free(accu, FALSE);
}

/* "S[string]" */
char *to_binstring_s (tagdb_value_t *v, size_t *size)
{
    GString *accu = g_string_new("S");
    g_string_append(accu, v->data.s);
    *size = accu->len + 1; // Note that this is okay here, only because the string contains no NULLs
    return g_string_free(accu, FALSE);

}

/* The keys and values are tagdb_value_t
   but the keys may only be strings or ints.
   The values may be anything */
/* "D[number of pairs][key 1][value 1][key 2][value 2]..." */
char *to_binstring_d (tagdb_value_t *v, size_t *size)
{
    GString *accu = g_string_new("D");
    *size += 1;
    if (!v->data.d)
    {
        HASH_INVALID:
        g_string_free(accu, TRUE);
        return NULL;
    }

    char *as_array = size_to_charp(g_hash_table_size(v->data.d));
    g_string_append_len(accu, as_array, sizeof(size_t));
    g_free(as_array);
    *size += TAGDB_VALUE_INT_SIZE;

    HL(v->data.d, it, k, val)
        tagdb_value_t *key = k;
        tagdb_value_t *value = val;

        if (key->type != tagdb_int_t &&
                key->type != tagdb_str_t)
        {
            goto HASH_INVALID;
        }

        binstring_t *k_string = tagdb_value_to_binstring(key);
        binstring_t *v_string = tagdb_value_to_binstring(value);

        g_string_append_len(accu, k_string->data, k_string->size);
        *size += k_string->size;

        g_string_append_len(accu, v_string->data, v_string->size);
        *size += v_string->size;
        g_free(k_string);
        g_free(v_string);
    HL_END;
    return g_string_free(accu, FALSE);
}

char *to_binstring_l (tagdb_value_t *v, size_t *size)
{
    GString *accu = g_string_new("L");
    *size += 1;

    char *siz = size_to_charp((size_t) g_list_length(v->data.l));
    g_string_append_len(accu, siz, sizeof(size_t));
    g_free(siz);
    *size += sizeof(size_t);

    LL(v->data.l, it)
        tagdb_value_t *value = it->data;

        binstring_t *v_string = tagdb_value_to_binstring(value);
        g_string_append_len(accu, v_string->data, v_string->size);
        *size += v_string->size;
        g_free(v_string);
    LL_END(it);
    return g_string_free(accu, FALSE);
}

char *to_binstring_b (tagdb_value_t *v, size_t *size)
{
    size_t v_size = v->data.b->size;
    char *res = g_malloc0(v_size);
    *size = v_size;
    g_memmove(v->data.b->data, res, v_size);
    return res;
}

v_to_bs value_to_bstring_converters[] = {
    to_binstring_d,
    to_binstring_l,
    to_binstring_i,
    to_binstring_s,
    to_binstring_b,
};

char *hash_to_string (GHashTable *hsh)
{
    if (hsh == NULL)
        return "";
    GString *accu = g_string_new("");
    HL(hsh, it, k, v)
        char *key = tagdb_value_to_str((tagdb_value_t*) k);
        char *value = tagdb_value_to_str((tagdb_value_t*) v);
        char *escaped = g_strescape(value, "");

        g_string_append_printf(accu, "%s\t", key);
        g_string_append_printf(accu, "%s\t", escaped);
        
        g_free(key);
        g_free(value);
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

binstring_t *tagdb_value_to_binstring (tagdb_value_t *value)
{
    binstring_t *bs = malloc(sizeof(binstring_t));
    int type = tagdb_value_get_type(value);
    size_t size = 0;
    bs->data = value_to_bstring_converters[type](value, &size);
    bs->size = size;
    return bs;
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
            return g_strdup_printf("%zd", value->data.i);
        case (tagdb_str_t):
            //printf("VALUE %s\n", value->s);
            return g_strdup(value->data.s);
        case (tagdb_err_t):
            if (value->data.s != NULL)
                return g_strdup_printf("ERROR: %s", value->data.s);
            else
                return g_strdup_printf("ERROR in tagdb_result_t"); 
        default:
            return g_strdup("BINDATA");
    }
}

// encapsualtes the object in a result type
/* Data is not copied for dict and list types since
   they are assumed to be heap-allocated anyway. 
   Strings are copied to defray the programmer cost from using string
   literals. */
result_t *encapsulate (int type, gpointer data)
{
    result_t *res = g_malloc(sizeof(result_t));
    switch (type)
    {
        case tagdb_dict_t:
            res->data.d = data;
            break;
        case tagdb_int_t:
            res->data.i = TO_64(data);
            break;
        case tagdb_str_t:
            res->data.s = g_strdup(data);
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
    tagdb_value_set_type(res, type);

    return res;
}

void binstring_destroy (binstring_t *bs)
{
    g_free(bs->data);
    bs->data = NULL;
    g_free(bs);
}

void query_destroy (query_t *q)
{
    //g_strfreev(q->argv);
    g_free(q);
}

void result_destroy (result_t *r)
{
    switch (r->type)
    {
        case tagdb_dict_t:
            g_hash_table_destroy(r->data.d);
            r->data.d = NULL;
            break;
        case tagdb_list_t:
            g_list_free_full(r->data.l, (GDestroyNotify)result_destroy);
        case tagdb_int_t:
            r->data.i = 0;
            break;
        case tagdb_str_t:
            g_free(r->data.s);
            r->data.s = NULL;
            break;
        case tagdb_err_t:
            g_free(r->data.s);
            r->data.s = NULL;
        case tagdb_bin_t:
            binstring_destroy(r->data.b);
            r->data.b = NULL;
        default:
            break;
    }
    g_free(r);
}

tagdb_value_t *default_value (int type)
{
    switch (type)
    {
        case tagdb_dict_t:
            return encapsulate(type, 
                g_hash_table_new_full((GHashFunc) tagdb_value_hash,(GEqualFunc) tagdb_value_equals,
                    (GDestroyNotify) result_destroy, (GDestroyNotify) result_destroy));
            break;
        case tagdb_int_t:
            return encapsulate(type, 0);
            break;
        case tagdb_str_t:
            return encapsulate(type, "");
            break;
        case tagdb_err_t:
            return encapsulate(type, "ERROR");
        case tagdb_list_t:
        default:
            return encapsulate(type, NULL);
            break;
    }
}

/* Shallow copy */
tagdb_value_t *copy_value (tagdb_value_t *v)
{
    gpointer data = NULL;
    switch (tagdb_value_get_type(v))
    {
        case tagdb_dict_t:
            data = v->data.d;
            break;
        case tagdb_int_t:
            data = TO_64P(v->data.i);
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

tagdb_value_t *value_from_stream (ScannerStream *stream)
{
    char s[2] = {0, 0};
    scanner_stream_read(stream, s, 1);
    int type = strv_index(type_strings, s);
    tagdb_value_t *res = default_value(type);
    switch (type)
    {
        case (tagdb_dict_t):
            {
                size_t size = sizeof(size_t);
                char tmp[size];
                size_t realsize = scanner_stream_read(stream, tmp, size);
                size_t n_pairs = charp_to_size(realsize, tmp);
                int i;

                for (i = 0; i < n_pairs; i++)
                {
                    tagdb_value_t *key = value_from_stream(stream);
                    tagdb_value_t *value = value_from_stream(stream);
                    g_hash_table_insert(res->data.d, key, value);
                }
            }
            break;
        case (tagdb_int_t):
            {
                char tmp[TAGDB_VALUE_INT_SIZE];
                scanner_stream_read(stream, tmp, TAGDB_VALUE_INT_SIZE);
                res->data.i = (TAGDB_VALUE_INT_TYPE) charp_to_size(TAGDB_VALUE_INT_SIZE, tmp);
            }
            break;
        case (tagdb_str_t):
            {
                GString *accu = g_string_new("");
                char c;
                while (( c = scanner_stream_getc(stream) ))
                {
                    g_string_append_c(accu, c);
                }
                res->data.s = g_string_free(accu, FALSE);
            }
            break;
        case (tagdb_list_t):
            {
                size_t size = sizeof(size_t);
                char tmp[size];
                size_t realsize = scanner_stream_read(stream, tmp, size);
                size_t n_items = charp_to_size(realsize, tmp);
                int i;

                for (i = 0; i < n_items; i++)
                {
                    tagdb_value_t *value = value_from_stream(stream);
                    res->data.l = g_list_prepend(res->data.l, value);
                }
                res->data.l = g_list_reverse(res->data.l);
            }
            break;
        case (tagdb_bin_t):
            {
                size_t size = sizeof(size_t);
                char tmp[size];
                scanner_stream_read(stream, tmp, size);
                size_t data_size = charp_to_size(size, tmp);
                char *data = g_malloc(data_size);
                scanner_stream_read(stream, data, data_size);
                res->data.b = data;
            }
            break;
        default:
            break;
    }
    return res;
}
