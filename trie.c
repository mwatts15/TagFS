#include <stdlib.h>
#include <glib.h>
#include "trie.h"
#include "key.h"
#include "util.h"
#include "log.h"

static void sort_key (trie_key_t key)
{
    if (!key) return;
    key_sort(key, cmp);
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

Trie *new_trie0 (bucket_id_t key, TrieBucket *items)
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

GList *trie_retrieve_bucket_l (Trie *t, trie_key_t key)
{
    return trie_bucket_as_list(trie_retrieve_bucket(t, key));
}

TrieBucket *trie_retrieve_bucket (Trie *t, trie_key_t key)
{
    if (key == NULL)
        return trie_node_bucket(t);
    sort_key(key);
    return _trie_retrieve_bucket(t, key);
}

TrieBucket *_trie_retrieve_bucket (Trie *t, trie_key_t key)
{
    Trie *my_trie = trie_retrieve_trie(t, key);
    if (my_trie)
        return trie_node_bucket(my_trie);
    return NULL;
}

Trie *trie_retrieve_trie (Trie *t, trie_key_t key)
{
    if (key == NULL)
    {
        return t;
    }
    sort_key(key);
    return _trie_retrieve_trie(t, key);
}

Trie *_trie_retrieve_trie (Trie *t, trie_key_t key)
{
    log_msg("retrieving %ld\n", key_ref(key,0));
    if (!t)
    {
        return NULL;
    }
    if (!key_ref(key,0))
    {
        log_msg("got it : %ld\n", trie_node_key(t));
        return t;
    }
    t = (Trie*) t->children;
    KL(key,i)
    {
        while ((t != NULL)
                &&(key_ref(key,i) != trie_node_key(t)))
        {
            t = t->next;
        }

        if (t == NULL)
        {
            // the key doesn't correspond to any node
            return NULL;
        }
        else
        {
            t = t->children;
        }
    }
    return t;
}

gpointer trie_retrieve (Trie *t, trie_key_t key, gpointer bucket_key)
{
    TrieBucket *tb = trie_retrieve_bucket(t, key);
    if (tb == NULL)
    {
        log_msg("trie bucket is null in trie_retrieve\n");
        return NULL;
    }
    return trie_bucket_lookup(tb, bucket_key);
}

void trie_insert (Trie *t, trie_key_t key, gpointer bucket_key, gpointer object)
{
    sort_key(key);
    TrieBucket *tb = _trie_make_bucket(t, key);
    trie_bucket_insert(tb, bucket_key, object);
}

gpointer trie_remove (Trie *t, trie_key_t key, gpointer bucket_key)
{
    sort_key(key);
    return _trie_remove(t, key, bucket_key);
}

gpointer _trie_remove (Trie *t, trie_key_t key, gpointer bucket_key)
{
    TrieBucket *tb = _trie_retrieve_bucket(t, key);
    return trie_bucket_remove(tb, bucket_key);
}

TrieBucket *trie_make_bucket (Trie *t, trie_key_t key)
{
    sort_key(key);
    return _trie_make_bucket(t, key);
}

TrieBucket *_trie_make_bucket (Trie *t, trie_key_t key)
{
    if (!t)
    {
        return NULL;
    }

    Trie *child = g_node_first_child(t);
    KL(key,i)
    {
        while ((child != NULL)
                &&(key_ref(key,i) != trie_node_key(child)))
        {
            child = child->next;
        }

        if (child == NULL)
        {
            // the key doesn't correspond to any node
            Trie *nt = new_trie0(key_ref(key,i), NULL);
            g_node_prepend(t, nt);
        }
        t = g_node_first_child(t);
        child = g_node_first_child(t);
    }
    return trie_node_bucket(t);
}
