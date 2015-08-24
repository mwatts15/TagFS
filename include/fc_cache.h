#ifndef FC_CACHE_H
#define FC_CACHE_H
#include <glib.h>
#include <inttypes.h>
#define DEFAULT_CACHE_SIZE 32u

typedef struct {
    GHashTable *tbl;
    GQueue *lru;
    unsigned size;
} FCCache;

typedef uint64_t cache_key_elem_t;

typedef struct {
    const char *name;
    uint64_t id;
} CacheEntry;

void fc_cache_init(FCCache *cache);
void fc_val_destroy(GArray *arr);
void fc_cache_destroy(FCCache *cache);
void fc_cache_insert(FCCache *cache, cache_key_elem_t key, const char *name, int id);
void fc_cache_invalidate(FCCache *cache, cache_key_elem_t key);


#endif /* FC_CACHE_H */

