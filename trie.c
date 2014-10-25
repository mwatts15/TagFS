#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include "trie.h"
#include "key.h"
#include "util.h"

gpointer _trie_remove (Trie *t, trie_key_t key, const char* bucket_key);
Trie *_trie_retrieve_trie (Trie *t, trie_key_t key);
TrieBucket *_trie_retrieve_bucket (Trie *t, trie_key_t key);
TrieBucket *_trie_make_bucket (Trie *t, trie_key_t key);

gboolean _node_collect (Trie *t, gpointer plist)
{
    GList *l = *((GList**) plist);
    GList *bucket_items = g_hash_table_get_values(trie_node_bucket(t));
    /* ( (key1 item1 item2 ...) (key2 item1 item2 ...) ...) */
    l = g_list_prepend(l, g_list_prepend(bucket_items, TO_SP(trie_node_key(t))));
    *((GList**) plist) = l;
    return FALSE;
}

void trie_bucket_destroy(TrieBucket *tb);
gboolean _trie_node_destroy (Trie *t, gpointer UNUSED)
{
    trie_bucket_destroy(((NodeData*)t->data)->items);
    g_free(t->data);
    return FALSE;
}

void trie_destroy (Trie *t)
{
    g_node_traverse (t, G_IN_ORDER, G_TRAVERSE_ALL,
                 -1,/*depth. unlimited */
                 _trie_node_destroy,NULL);
    g_node_destroy(t);
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

Trie *new_trie()
{
    Trie *res = new_trie0(0, NULL);
    return res;
}

GList *trie_bucket_as_list (TrieBucket *tb)
{
    if (tb)
    {
        return g_hash_table_get_values((GHashTable*)tb);
    }
    else
    {
        return NULL;
    }
}

gpointer trie_retrieve (Trie *t, trie_key_t key, const char* bucket_key)
{
    TrieBucket *tb = trie_retrieve_bucket(t, key);
    if (tb == NULL)
    {
        return NULL;
    }
    return trie_bucket_lookup(tb, (gpointer)bucket_key);
}

void trie_insert (Trie *t, trie_key_t key, const char* bucket_key, gpointer object)
{
    TrieBucket *tb = _trie_make_bucket(t, key);

    trie_bucket_insert(tb, (gpointer)bucket_key, object);
}

gpointer trie_remove (Trie *t, trie_key_t key, const char* bucket_key)
{
    return _trie_remove(t, key, bucket_key);
}

gpointer _trie_remove (Trie *t, trie_key_t key, const char* bucket_key)
{
    TrieBucket *tb = _trie_retrieve_bucket(t, key);
    return trie_bucket_remove(tb, (gpointer)bucket_key);
}

GList *trie_retrieve_bucket_l (Trie *t, trie_key_t key)
{
    return trie_bucket_as_list(trie_retrieve_bucket(t, key));
}

TrieBucket *trie_retrieve_bucket (Trie *t, trie_key_t key)
{
    if (key == NULL)
        return trie_node_bucket(t);
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
    return _trie_retrieve_trie(t, key);
}

Trie *_trie_retrieve_trie (Trie *t, trie_key_t key)
{
    if (!t)
    {
        return NULL;
    }

    if (key_is_empty(key))
    {
        return t;
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
            return NULL;
        }
        t = g_node_first_child(t);
        child = g_node_first_child(t);
    }
    return t;
}

TrieBucket *trie_make_bucket (Trie *t, trie_key_t key)
{
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

void trie_bucket_destroy(TrieBucket *tb)
{
    g_hash_table_destroy(tb);
}
