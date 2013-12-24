#include "stage.h"

static void sort_key (tagdb_key_t key)
{
    if (!key) return;
    key_sort(key, cmp);
}

/* Staging tags created with mkdir */
Stage *new_stage ()
{
    Stage *res = g_try_malloc0(sizeof(Stage));
    if (!res)
        return NULL;
    res->data = new_trie();
    return res;
}

Tag* stage_lookup (Stage *s, tagdb_key_t position, char *name)
{
    sort_key(position);
    return trie_retrieve(s->data, position, name);
}

void stage_add (Stage *s, tagdb_key_t position, char *name, gpointer item)
{
    sort_key(position);
    trie_insert(s->data, position, name, item);
}

void stage_remove (Stage *s, tagdb_key_t position, char *name)
{
    sort_key(position);
    trie_remove(s->data, position, name);
}

GList *stage_list_position (Stage *s, tagdb_key_t position)
{
    sort_key(position);
    return trie_retrieve_bucket_l(s->data, position);
}
