#include <glib.h>
#include <inttypes.h>
#include "util.h"
#include "fc_cache.h"

void fc_cache_init(FCCache *cache)
{
    cache->tbl = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)fc_val_destroy);
    cache->lru = g_queue_new();
    cache->size = DEFAULT_CACHE_SIZE;
}

void fc_cache_invalidate(FCCache *cache, cache_key_elem_t key)
{
    g_hash_table_remove(cache->tbl, TO_64P(key));
    g_queue_remove(cache->lru, TO_64P(key));
}

void fc_cache_insert(FCCache *cache, cache_key_elem_t key, const char *name, int id)
{
    GArray *ents = (GArray*)g_hash_table_lookup(cache->tbl, TO_64P(key));
    if (ents == NULL)
    {
        ents = g_array_sized_new(FALSE, FALSE, sizeof(CacheEntry), 32);
        g_hash_table_insert(cache->tbl, TO_64P(key), ents);
    }

    static CacheEntry ent;
    ent.name = name;
    ent.id = id;

    g_array_append_val(ents, ent);
}

void fc_cache_destroy(FCCache *cache)
{
    g_hash_table_destroy(cache->tbl);
    g_queue_free(cache->lru);
}

void fc_val_destroy(GArray *arr)
{
    g_array_free(arr, TRUE);
}
