#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "types.h"
#include "util.h"
#include "set_ops.h"
#include "stream.h"

#define append0(_g_string) g_string_append_c(_g_string, '\0')
const char type_syms[] = {
    'D',
    'L',
    'I',
    'S',
    'B',
    'E',
    '*',
    '\0'
};

const char *type_strings[] = {
    "DICT",
    "LIST",
    "INT",
    "STRING",
    "BYTESTRING"
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
            return 2;
    }
}

void tagdb_value_set_type (tagdb_value_t *v, int type)
{
    v->type = type;
}

int tagdb_value_get_type (tagdb_value_t *v)
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
    {
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
    } HL_END;
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
    {
        tagdb_value_t *value = it->data;

        binstring_t *v_string = tagdb_value_to_binstring(value);
        g_string_append_len(accu, v_string->data, v_string->size);
        *size += v_string->size;
        g_free(v_string);
    } LL_END;
    return g_string_free(accu, FALSE);
}

char *to_binstring_b (tagdb_value_t *v, size_t *size)
{
    int info_offset = 1 + sizeof(size_t);
    size_t v_size = v->data.b->size + info_offset;
    char *res = g_malloc0(v_size);
    *size = v_size;
    res[0] = 'B';

    char *siz = size_to_charp(v->data.b->size);
    memcpy(res + 1, siz, sizeof(size_t));
    g_free(siz);

    g_memmove(v->data.b->data, res + info_offset, v->data.b->size);
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
    {
        char *key = tagdb_value_to_str((tagdb_value_t*) k);
        char *value = tagdb_value_to_str((tagdb_value_t*) v);
        char *escaped = g_strescape(value, "");

        g_string_append_printf(accu, "%s\t", key);
        g_string_append_printf(accu, "%s\t", escaped);

        g_free(key);
        g_free(value);
        g_free(escaped);
    } HL_END;
    g_string_truncate(accu, accu->len - 1); // chop off that tab
    char *res = g_strdup(accu->str);
    g_string_free(accu, TRUE);
    return res;
}

char *list_to_string (GList *l)
{
    GString *accu = g_string_new("");
    LL(l, it)
    {
        char *this = tagdb_value_to_str((tagdb_value_t*) it->data);
        char *escaped = g_strescape(this, "");

        g_string_append_printf(accu, "%s\t", this);

        g_free(escaped);
        g_free(this);
    } LL_END;
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
    tagdb_value_t *res = g_malloc(sizeof(tagdb_value_t));
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
        case (tagdb_err_t):
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
    binstring_t *bs = g_malloc(sizeof(binstring_t));
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
            return g_strdup(value->data.s);
        case (tagdb_bin_t):
            return g_memdup(value->data.b->data, value->data.b->size);
        case (tagdb_err_t):
            return g_strdup_printf("ERROR: %s", value->data.s);
        default:
            return g_strdup("NO SUCH TYPE");
    }
}

// encapsualtes the object in a result type
result_t *encapsulate (char *type_str, gpointer data)
{
    result_t *res = g_malloc(sizeof(result_t));
    int type = strchr(type_syms, type_str[0]) - type_syms;
    switch (type)
    {
        case tagdb_dict_t:
            {
                GHashTable *d = tagdb_value_dict_new();
                char *k_type = type_str + 1;
                char *v_type = type_str + 2;
                LL(data, it)
                {
                    GList *pair = it->data;
                    result_t *key = encapsulate(k_type, pair->data);
                    result_t *val = encapsulate(v_type, pair->next->data);
                    g_hash_table_insert(d, key, val);
                } LL_END;
                res->data.d = d;
                g_list_free_full(data, (GDestroyNotify) g_list_free);
            }
            break;
        case tagdb_list_t:
            {
                GList *l = NULL;
                char *v_type = type_str + 1;
                LL(data, it)
                {
                    it->data = encapsulate(v_type, it->data);
                } LL_END;
                res->data.l = l;
            }
        case tagdb_int_t:
            res->data.i = TO_64(data);
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
    if (!r) return;
    switch (r->type)
    {
        case tagdb_dict_t:
            g_hash_table_destroy(r->data.d);
            r->data.d = NULL;
            break;
        case tagdb_list_t:
            g_list_free(r->data.l);//g_list_free_full(r->data.l, (GDestroyNotify)result_destroy);
            r->data.l = NULL;
            break;
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
            break;
        case tagdb_bin_t:
            binstring_destroy(r->data.b);
            r->data.b = NULL;
            break;
        default:
            break;
    }
    g_free(r);
}

gboolean tagdb_value_equals_data (tagdb_value_t *lhs, gpointer rhs)
{
    switch (lhs->type)
    {
        case tagdb_str_t:
            return g_str_equal(lhs->data.s, (char*) rhs);
        case tagdb_int_t:
            return (lhs->data.i == TO_64(rhs));
        default:
            return FALSE;
    }
}

int tagdb_value_dict_get_key_type (tagdb_value_t *dict)
{
    int type = -1;
    if (dict && dict->type == tagdb_dict_t)
    {
        HL(dict->data.d, it, k, v)
        {
            type = tagdb_value_get_type(k);
            break;
        } HL_END;
    }
    return type;
}

/* A utility function for use with tagdb_dict_t hashes
   The lhs is assumed to be tagdb_value_t and the rhs is either
   string or a string representation of a decimal integer
   according to the lhs type */
tagdb_value_t *tagdb_value_dict_lookup_data (tagdb_value_t *dict, char *data)
{
    tagdb_value_t *res = NULL;
    if (dict && dict->type == tagdb_dict_t)
    {
        int key_type = tagdb_value_dict_get_key_type(dict);
        tagdb_value_t *rhs = tagdb_str_to_value(key_type, data);
        res = g_hash_table_lookup(dict->data.d, rhs);
        result_destroy(rhs);
    }
    return res;
}

GHashTable *tagdb_value_dict_new ()
{
    return g_hash_table_new_full(
            (GHashFunc) tagdb_value_hash,
            (GEqualFunc) tagdb_value_equals,
            (GDestroyNotify) result_destroy,
            (GDestroyNotify) result_destroy
            );
}

tagdb_value_t *default_value (int type)
{
    result_t *res = g_malloc(sizeof(result_t));
    switch (type)
    {
        case tagdb_dict_t:
            res->data.d = tagdb_value_dict_new();
            break;
        case tagdb_list_t:
            res->data.l = NULL;
            break;
        case tagdb_int_t:
            res->data.i = 0;
            break;
        case tagdb_str_t:
            res->data.s = g_strdup("$");
            break;
        case tagdb_err_t:
            res->data.s = g_strdup("ERROR");
            break;
        default:
            res->data.i = 0;
            type = 0;
            break;
    }
    tagdb_value_set_type(res, type);
    return res;
}

/* Shallow copy */
tagdb_value_t *copy_value (tagdb_value_t *v)
{
    result_t *res = g_malloc(sizeof(result_t));
    res->data = v->data;
    res->type = v->type;
    return res;
}

tagdb_value_t *tagdb_value_from_stream (ScannerStream *stream)
{
    char c = scanner_stream_getc(stream);
    int type = strchr(type_syms, c) - type_syms;
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
                    tagdb_value_t *key = tagdb_value_from_stream(stream);
                    tagdb_value_t *value = tagdb_value_from_stream(stream);
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
                g_string_append_c(accu, '\0');
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
                    tagdb_value_t *value = tagdb_value_from_stream(stream);
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
                res->data.b->data = data;
                res->data.b->size = data_size;
            }
            break;
        default:
            break;
    }
    return res;
}

gboolean tagdb_value_is_error (tagdb_value_t *value)
{
    return (!value || tagdb_value_get_type(value) == tagdb_err_t);
}

const char *tagdb_value_strerror (tagdb_value_t *value)
{
    if (tagdb_value_is_error(value))
    {
        return value->data.s;
    }
    return NULL;
}

GList *tagdb_value_extract_list (tagdb_value_t *v)
{
    if (tagdb_value_get_type(v) == tagdb_list_t)
    {
        return v->data.l;
    }
    return NULL;
}

TAGDB_VALUE_INT_TYPE tagdb_value_extract_int (tagdb_value_t *v)
{
    if (tagdb_value_get_type(v) == tagdb_int_t)
    {
        return v->data.i;
    }
    return 0;
}

void res_info (result_t *r, printer p)
{
    if (!r)
    {
        return;
    }
    p("result info:\n");
    p("\ttype: %s\n", type_strings[r->type]);
    p("\tdata: ");
    char *str = tagdb_value_to_str(r);
    p("%s\n", str);
    g_free(str);
}
