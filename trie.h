#ifndef TRIE_H
#define TRIE_H
#include <glib.h>
#include "key.h"

/* A node in the actual trie data structure */
typedef GNode Trie;

typedef unsigned long bucket_id_t;
typedef tagdb_key_t trie_key_t;

typedef GHashTable TrieBucket;

/* Data for each node in the trie */
typedef struct
{
    bucket_id_t key;
    TrieBucket *items;
} NodeData;

Trie *new_trie();
void trie_destroy (Trie *t);

/* Objects in a trie are accessed by their keys.
   So to add, remove, or access an item in the trie
   you need to get the key path for that object as
   well as its identifier for the bucket at the end of
   that path. */

gpointer trie_remove (Trie *t, trie_key_t key, gconstpointer bucket_key);
/* Inserts the item at the keyed position, creating
   the path to the desired bucket if needed */
void trie_insert (Trie *t, trie_key_t key, gconstpointer bucket_key, gpointer object);


gpointer trie_retrieve (Trie *t, trie_key_t key, gconstpointer bucket_key);
Trie *trie_retrieve_trie (Trie *t, trie_key_t key);

GList *trie_retrieve_bucket_l (Trie *t, trie_key_t key);
TrieBucket *trie_retrieve_bucket (Trie *t, trie_key_t key);


/* Puts a trie bucket in the specified path. The last key is
   the one that gets created if there isn't one already.
   Mostly for making empty directories in TagFS */
TrieBucket *trie_make_bucket (Trie *t, trie_key_t key);

/* Gets the key value for the node */
#define trie_node_key(t) (((NodeData*) t->data)->key)
#define trie_node_bucket(t) (((NodeData*) t->data)->items)

TrieBucket *new_trie_bucket ();

/* Inserts the given object into a TrieBucket */
void trie_bucket_insert (TrieBucket *tb, gpointer key, gpointer object);

gpointer trie_bucket_lookup (TrieBucket *tb, gpointer key);

gpointer trie_bucket_remove (TrieBucket *tb, gpointer key);

GList *trie_bucket_as_list (TrieBucket *tb);

GList *trie_children (Trie *t);

#define trie_is_root G_NODE_IS_ROOT
#endif /* TRIE_H */
