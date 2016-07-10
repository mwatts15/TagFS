#include <glib.h>
#include <assert.h>
#include "tagdb_util.h"
#include "key.h"
#include "log.h"

tagdb_key_t key_new (void)
{
    return g_array_sized_new(TRUE, TRUE, sizeof(key_elem_t), 10);
}

tagdb_key_t make_key (key_elem_t *args, int nkeys)
{
    tagdb_key_t res = key_new();
    for (int i = 0; i < nkeys; i++)
    {
        key_push_end(res, args[i]);
    }
    return res;
}

void key_destroy (tagdb_key_t k)
{
    if (k)
    {
        g_array_free(k, TRUE);
    }
}

key_elem_t key_ref (tagdb_key_t k, int index)
{
    return g_array_index(k, key_elem_t, index);
}

void key_push_end (tagdb_key_t k, key_elem_t e)
{
    g_array_append_val(k, e);
}

key_elem_t key_pop_front (tagdb_key_t k)
{
    key_elem_t res = g_array_index(k, key_elem_t, 0);
    g_array_remove_index(k, 0);
    return res;
}

tagdb_key_t key_copy (tagdb_key_t k)
{
    tagdb_key_t res = key_new();
    KL(k,i)
    {
        key_push_end(res, key_ref(k, i));
    }
    return res;
}

void key_insert (tagdb_key_t k, key_elem_t e)
{
    if (key_is_empty(k))
    {
        g_array_insert_val(k, 0, e);
        return;
    }

    KL(k, i)
    {
        if (e < key_ref(k,i))
        {
            g_array_insert_val(k, i, e);
            break;
        }
    } KL_END;
}

int key_is_empty (tagdb_key_t k)
{
    assert(k);
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

gboolean key_equal(tagdb_key_t k, tagdb_key_t g)
{
    int sop = 1;
    KL(k, i)
    {
        int dop = 0;
        key_elem_t z = key_ref(k, i);
        KL(g, j)
        {
            key_elem_t p = key_ref(g,j);
            if (p == z)
            {
                dop = 1;
                break;
            }
        } KL_END;

        if (!dop)
        {
            sop = 0;
            break;
        }
    } KL_END;

    return sop;
}

void print_key (tagdb_key_t k)
{
    printf("<<");
    KL(k, i)
    {
        printf("%lld", key_ref(k,i));
        if (i < (key_length(k) - 1))
        {
            printf(" ");
        }
    } KL_END;
    printf(">>");
}

void log_key (tagdb_key_t k)
{
    lock_log();
    log_msg("<<");
    KL(k, i)
    {
        log_msg("%lld ", key_ref(k,i));
    } KL_END;
    log_msg(">>\n");
    unlock_log();
}

gboolean key_starts_with(tagdb_key_t key, tagdb_key_t starts_with)
{
    if (key_length(key) < key_length(starts_with))
    {
        return FALSE;
    }
    else
    {
        KL(key, i)
        {
            if (key_ref(key, i) != key_ref(starts_with, i))
            {
                return FALSE;
            }
        } KL_END;
        return TRUE;
    }
}

gboolean key_contains(tagdb_key_t key, key_elem_t elem)
{
    KL(key, i)
    {
        if (key_ref(key, i) == elem)
        {
            return TRUE;
        }
    } KL_END;
    return FALSE;
}
