#include <glib.h>
#include <assert.h>
#include "tagdb_util.h"
#include "key.h"
#include "log.h"

tagdb_key_t key_new (void)
{
    return g_array_new(TRUE, TRUE, sizeof(key_elem_t));
}

tagdb_key_t make_key (key_elem_t *args, int nkeys)
{
    tagdb_key_t res = key_new();
    for (int i = 0; i < nkeys; i++)
    {
        key_insert(res, args[i]);
    }
    return res;
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

void key_insert (tagdb_key_t k, key_elem_t e)
{
    KL(k, i)
    {
        if (e > key_ref(k,i))
        {
            g_array_insert_val(k, i, e);
            break;
        }
    } KL_END;
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

/* From: http://stackoverflow.com/questions/8317508/hash-function-for-a-string
 */
#define A 54059 /* a prime */
#define B 76963 /* another prime */
#define C 86969 /* yet another prime */
guint key_hash(const tagdb_key_t s)
{
   guint h = 31 /* also prime */;
   KL(s, i)
   {
     h = (h * A) ^ (key_ref(s,i) * B);
   } KL_END;
   return h; // or return h % C;
}

void print_key (tagdb_key_t k)
{
    printf("<<");
    KL(k, i)
    {
        printf("%lld ", key_ref(k,i));
    } KL_END;
    printf(">>\n");
}

void log_key (tagdb_key_t k)
{
    log_msg("<<");
    KL(k, i)
    {
        log_msg("%lld ", key_ref(k,i));
    } KL_END;
    log_msg(">>\n");
}

