#include <stdlib.h>
#include <glib.h>
#include "trie.h"
#include "util.h"
#include "log.h"

void fix_key (gulong *key)
{
    if (!key) return;
    int i = 0;
    while (key[i] != 0) i++;
    qsort(key, i, sizeof(gulong), cmp);
}

gboolean _node_collect (Trie *t, gpointer plist)
{
    printf("collecting %ld onto %p\n", trie_node_key(t), plist);
    GList *l = *((GList**) plist);
    GList *bucket_items = g_hash_table_get_values(trie_node_bucket(t));
    /* ( (key1 item1 item2 ...) (key2 item1 item2 ...) ...) */
    l = g_list_prepend(l, g_list_prepend(bucket_items, TO_SP(trie_node_key(t))));
    *((GList**) plist) = l;
    return FALSE;
}
/* returns the children of the Trie in the form
   ((key (bucket_list)) ...)
   where each key is actually a key part
   */
GList *trie_children (Trie *t)
{
    GList *l = NULL;
    g_node_traverse(t, G_IN_ORDER, G_TRAVERSE_ALL, -1, _node_collect, &l);
    return l;
}

void trie_bucket_insert (TrieBucket *tb, gpointer key, gpointer object)
{
    g_hash_table_insert(tb, key, object);
}

TrieBucket *new_trie_bucket ()
{
    return (TrieBucket*) g_hash_table_new(g_str_hash, g_str_equal);
}

gpointer trie_bucket_lookup (TrieBucket *tb, gpointer key)
{
    return g_hash_table_lookup(tb, key);
}

gpointer trie_bucket_remove (TrieBucket *tb, gpointer key)
{
    gpointer res = g_hash_table_lookup(tb, key);
    g_hash_table_remove(tb, key);
    return res;
}

Trie *new_trie0 (gulong key, TrieBucket *items)
{
    NodeData *nd = malloc(sizeof(NodeData));
    nd->key = key;
    if (!items)
        nd->items = new_trie_bucket();
    else
        nd->items = items;
    return (Trie*) g_node_new((gpointer) nd);
}

GList *trie_bucket_as_list (TrieBucket *tb)
{
    return g_hash_table_get_values((GHashTable*)tb);
}

GList *trie_retrieve_bucket_l (Trie *t, gulong *key)
{
    return trie_bucket_as_list(trie_retrieve_bucket(t, key));
}

TrieBucket *trie_retrieve_bucket (Trie *t, gulong *key)
{
    if (key == NULL)
        return trie_node_bucket(t);
    fix_key(key);
    return _trie_retrieve_bucket(t, key);
}

TrieBucket *_trie_retrieve_bucket (Trie *t, gulong *key)
{
    Trie *my_trie = trie_retrieve_trie(t, key);
    if (my_trie)
        return trie_node_bucket(my_trie);
    return NULL;
}

Trie *trie_retrieve_trie (Trie *t, gulong *key)
{
    if (key == NULL)
    {
        return t;
    }
    fix_key(key);
    return _trie_retrieve_trie(t, key);
}

Trie *_trie_retrieve_trie (Trie *t, gulong *key)
{
    log_msg("retrieving %ld\n", key[0]);
    if (!t)
    {
        return NULL;
    }
    if (!key[0])
    {
        log_msg("got it : %ld\n", trie_node_key(t));
        return t;
    }
    Trie *child = (Trie*) t->children;
    while (child != NULL)
    {
        if (key[0] == trie_node_key(child))
        {
            return _trie_retrieve_trie(child, key + 1);
        }
        child = child->next;
    }
    return NULL;
}

gpointer trie_retrieve (Trie *t, gulong *key, gpointer bucket_key)
{
    TrieBucket *tb = trie_retrieve_bucket(t, key);
    if (tb == NULL)
    {
        _log_level = 0;
        log_msg("trie bucket is null in trie_retrieve\n");
        return NULL;
    }
    return trie_bucket_lookup(tb, bucket_key);
}

void _trie_insert (Trie *t, gulong *key, gpointer bucket_key, gpointer object)
{
    TrieBucket *tb = _trie_make_bucket(t, key);
    trie_bucket_insert(tb, bucket_key, object);
}

gpointer trie_remove (Trie *t, gulong *key, gpointer bucket_key)
{
    fix_key(key);
    return _trie_remove(t, key, bucket_key);
}

gpointer _trie_remove (Trie *t, gulong *key, gpointer bucket_key)
{
    TrieBucket *tb = _trie_retrieve_bucket(t, key);
    return trie_bucket_remove(tb, bucket_key);
}

TrieBucket *trie_make_bucket (Trie *t, gulong *key)
{
    fix_key(key);
    return _trie_make_bucket(t, key);
}

TrieBucket *_trie_make_bucket (Trie *t, gulong *key)
{
    if (!t)
    {
        return NULL;
    }
    if (!key[0])
    {
        return trie_node_bucket(t);
    }
    Trie *child = (Trie*) t->children;
    while (child != NULL)
    {
        if (key[0] == trie_node_key(child))
        {
            return _trie_make_bucket (child, key + 1);
        }
        child = child->next;
    }
    Trie *nt = new_trie0(key[0], NULL);
    g_node_insert(t, 0, nt);
    return _trie_make_bucket(nt, key + 1);
}
