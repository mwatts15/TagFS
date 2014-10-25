#include "stage.h"
#include "key.h"

#define trie_index(__t) ((__t)->index)

/* Staging tags created with mkdir */
Stage *new_stage ()
{
    Stage *res = g_try_malloc0(sizeof(Stage));
    trie_index(res) = g_hash_table_new(g_direct_hash, g_direct_equal);
    if (!res)
        return NULL;
    res->data = new_trie();
    return res;
}

Tag* stage_lookup (Stage *s, tagdb_key_t position, const char *name)
{
    return trie_retrieve(s->data, position, name);
}

void stage_add (Stage *s, tagdb_key_t position, AbstractFile *item)
{
    const char *name = abstract_file_get_name(item);

    tagdb_key_t index_key = key_copy(position);
    key_push_end(index_key, get_file_id(item));
    KL(index_key, i)
    {
        gpointer k = TO_SP(key_ref(index_key, i));
        GList *l = g_hash_table_lookup(trie_index(s), k);
        l = g_list_prepend(l, key_copy(position));
        g_hash_table_insert(trie_index(s), k, l);
    }
    key_destroy(index_key);

    trie_insert(s->data, position, name, item);
}

GList *_trie_index_lookup(Stage *t, const char* name)
{
    return g_hash_table_lookup(trie_index(t), name);
}

void trie_remove_by_bucket_key (Stage *t, AbstractFile *s)
{
    gpointer id = TO_SP(get_file_id(s));
    GList *l = g_hash_table_lookup(trie_index(t), id);
    LL(l, it)
    {
        stage_remove(t, it->data, s);
    }LL_END;
    g_hash_table_remove(trie_index(t), id);
    g_list_free_full(l, (GDestroyNotify)key_destroy);
}

void stage_remove (Stage *s, tagdb_key_t position, AbstractFile *f)
{
    trie_remove(s->data, position, abstract_file_get_name(f));
}

void stage_remove_tag (Stage *s, AbstractFile *t)
{
    trie_remove_by_bucket_key(s, t);
}

GList *stage_list_position (Stage *s, tagdb_key_t position)
{
    return trie_retrieve_bucket_l(s->data, position);
}

void stage_destroy (Stage *s)
{
    HL(trie_index(s), it, k, v)
    {
        g_list_free_full(v, (GDestroyNotify) key_destroy);
    } HL_END
    g_hash_table_destroy(trie_index(s));
    trie_destroy(s->data);
    g_free(s);
}
