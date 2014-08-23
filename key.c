#include <glib.h>
#include <assert.h>
#include "tagdb_util.h"
#include "key.h"
#include "log.h"

tagdb_key_t key_new (void)
{
    return g_array_new(TRUE, TRUE, sizeof(key_elem_t));
}

void key_destroy (tagdb_key_t k)
{
    assert(k);
    g_array_free(k, TRUE);
}

key_elem_t key_ref (tagdb_key_t k, int index)
{
    return g_array_index(k, key_elem_t, index);
}

void key_push_end (tagdb_key_t k, key_elem_t e)
{
    g_array_append_val(k, e);
}

int key_is_empty (tagdb_key_t k)
{
    return (k->len == 0);
}

void key_sort (tagdb_key_t k, GCompareFunc c)
{
    g_array_sort(k,c);
}

guint key_length (tagdb_key_t k)
{
    return k->len;
}

void log_key (tagdb_key_t k)
{
    log_msg("<<");
    KL(k, i)
    {
        log_msg("%ld ", key_ref(k,i));
    } KL_END;
    log_msg(">>\n");
}

