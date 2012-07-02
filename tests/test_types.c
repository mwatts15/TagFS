#include <stdio.h>
#include <limits.h>
#include "util.h"
#include "test_util.h"
#include "types.h"

/* arguments are all (user) strings
   for simplicities sake */
GList *new_value_list (int type, ...)
{
    va_list args;
    GList *res = NULL;

    va_start(args, type);
    gchar *item;
    for (item = va_arg(args, char*);
            item != NULL; item = va_arg(args, char*))
    {
        res = g_list_prepend(res, tagdb_str_to_value(type, item));
    }
    return g_list_reverse(res);
}

int test_type (int type, gpointer data, FILE *f)
{
    result_t *value = encapsulate(type, data);
    size_t size = 0;
    char *s = value_to_bstring_converters[type](value, &size);
    fwrite(s, 1, size, f);
    g_free(s);

    result_destroy(value);
    return 0;
}

int main ()
{
    FILE *f = fopen("ttypes_out", "w+");
    gpointer data = new_value_list(tagdb_str_t, "one", "two", "three", "four", "five", "six", NULL);
    test_type(tagdb_list_t, data, f);

    test_type(tagdb_int_t, TO_SP(1234567), f);
    test_type(tagdb_int_t, TO_SP(G_MAXLONG), f);
    test_type(tagdb_int_t, TO_SP(G_MINLONG), f);

    data = g_hash_table_new_full(tagdb_value_hash, tagdb_value_equals,
            (GDestroyNotify) result_destroy, (GDestroyNotify) result_destroy);
    int i = 0;
    for (i = 0; i < 10; i++)
    {
        g_hash_table_insert((GHashTable*) data, encapsulate(tagdb_int_t, TO_P(i)),
                encapsulate(tagdb_str_t, g_strdup_printf("value::%d", i)));
    }
    test_type(tagdb_dict_t, data, f);
    ScannerStream *ss = scanner_stream_new(FILE_S, f);

    while (!scanner_stream_is_empty(ss))
    {
        tagdb_value_t *this = value_from_stream(ss);
        char *str = tagdb_value_to_str(this);
        printf("got value : \"%s\"\n", str);
        result_destroy(this);
        g_free(str);
    }
    return 0;
}
