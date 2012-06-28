#ifndef TRIE_H
#define TRIE_H
#include <glib.h>

/* A node in the actual trie data structure */
typedef GNode Trie;

typedef GHashTable TrieBucket;

/* Data for each node in the trie */
typedef struct
{
    gulong key;
    TrieBucket *items;
} NodeData;

#define new_trie() new_trie0(0, NULL);
Trie *new_trie0 (gulong key, TrieBucket *items);

/* Objects in a trie are accessed by their keys.
   So to add, remove, or access an item in the trie
   you need to get the key path for that object as
   well as its identifier for the bucket at the end of
   that path. */

/* sorts a key so the lookup works correctly */
void fix_key (gulong *key);

/* Inserts an object into the trie at the keyed location given
   as an array of unsigned long int values. */
#define SORT_FIRST(func,t,k,...) \
    do { \
    fix_key(k); \
    func(t,k,__VA_ARGS__); \
    } while (0)

#define trie_insert(...) SORT_FIRST(_trie_insert, __VA_ARGS__)

gpointer trie_remove (Trie *t, gulong *key, gpointer bucket_key);
/* Inserts the item at the keyed position, creating 
   the path to the desired bucket if needed */
void _trie_insert (Trie *t, gulong *key, gpointer bucket_key, gpointer object);

/* Removes the bucket at the keyed position, and returns the
   bucket for further processing */
gpointer _trie_remove (Trie *t, gulong *key, gpointer bucket_key);

gpointer trie_retrieve (Trie *t, gulong *key, gpointer bucket_key);
Trie *trie_retrieve_trie (Trie *t, gulong *key);
Trie *_trie_retrieve_trie (Trie *t, gulong *key);

GList *trie_retrieve_bucket_l (Trie *t, gulong *key);
TrieBucket *trie_retrieve_bucket (Trie *t, gulong *key);

/* Retrieves the bucket at the keyed position */
TrieBucket *_trie_retrieve_bucket (Trie *t, gulong *key);

/* Puts a trie bucket in the specified path. The last key is
   the one that gets created if there isn't one already.
   Mostly for making empty directories in TagFS */
TrieBucket *trie_make_bucket (Trie *t, gulong *key);
TrieBucket *_trie_make_bucket (Trie *t, gulong *key);

/* Gets the key value for the node */
#define trie_node_key(t) ((NodeData*) t->data)->key
#define trie_node_bucket(t) ((NodeData*) t->data)->items

TrieBucket *new_trie_bucket ();

/* Inserts the given object into a TrieBucket */
void trie_bucket_insert (TrieBucket *tb, gpointer key, gpointer object);

gpointer trie_bucket_lookup (TrieBucket *tb, gpointer key);

gpointer trie_bucket_remove (TrieBucket *tb, gpointer key);

GList *trie_bucket_as_list (TrieBucket *tb);

GList *trie_children (Trie *t);

#define trie_is_root G_NODE_IS_ROOT
#endif /* TRIE_H */
