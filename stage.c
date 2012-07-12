#include "stage.h"

/* Staging tags created with mkdir */
Stage *new_stage ()
{
    Stage *res = g_try_malloc0(sizeof(Stage));
    if (!res)
        return NULL;
    res->data = new_trie();
    return res;
}

int stage_lookup (Stage *s, trie_key_t position, char *name)
{
    return (trie_retrieve(s->data, position, name) != NULL);
}

void stage_add (Stage *s, trie_key_t position, char *name, gpointer item)
{
    trie_insert(s->data, position, name, item);
}

void stage_remove (Stage *s, trie_key_t position, char *name)
{
    trie_remove(s->data, position, name);
}

GList *stage_list_position (Stage *s, trie_key_t position)
{
    return trie_retrieve_bucket_l(s->data, position);
}
