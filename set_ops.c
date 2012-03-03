#include "util.h"
#include "set_ops.h"
#include <stdlib.h>

// can't return 0, else same size hashses
// would get overwritten
gint hash_size_cmp (gpointer a, gpointer b)
{
    int asize = g_hash_table_size((GHashTable*) a);
    int bsize = g_hash_table_size((GHashTable*) b);
    if (asize < bsize)
    {
        return 1;
    }
    else
        return -1;
}

// assumes a is the shorter
GHashTable *_intersect_s (GHashTable *a, GHashTable *b)
{
    GHashTableIter it;
    gpointer key;
    gpointer value;
    GHashTable *res = g_hash_table_new(g_direct_hash, g_direct_equal);

    g_hash_table_iter_init(&it, a);
    while (g_hash_table_iter_next(&it, &key, &value))
    {
        if (g_hash_table_lookup(b, key) != NULL)
        {
            g_hash_table_insert(res, key, value);
        }
    }
    return res;
}

GHashTable *set_intersect_s (GHashTable *a, GHashTable *b)
{
    if (a == NULL)
    {
        return b;
    }
    if (b == NULL)
    {
        return a;
    }

    GHashTable *tmp;

    if (g_hash_table_size(a) > g_hash_table_size(b))
    {
        tmp = a;
        a = b;
        b = tmp;
    }
    return _intersect_s(a, b);
}

GHashTable *set_intersect (GList *tables)
{
    // do an inorder traversal of the tree, doing a
    // lookup all the way to the last leaf on the right
    // stop looking as soon as a lookup fails
    if (tables == NULL)
    {
        return NULL;
    }
    if (g_list_next(tables) == NULL)
    {
        return tables->data;
    }
    GHashTable *res = NULL;
    GHashTable *tmp;
    while (tables != NULL)
    {
        tmp = set_intersect_s(tables->data, res);
        res = tmp;
        tables = tables->next;
    }
    return res;
}

GHashTable *set_intersect_p (GHashTable *a, ...)
{
    // put the tables into sorted order in a GTree, then
    // do _intersect_p which expects the smallest table
    // as the first
    va_list args;
    va_start(args, a);
    GHashTable *arg = va_arg(args, GHashTable*);
    GList *tables = g_list_prepend(NULL, a);
    while (arg != NULL)
    {
        tables = g_list_prepend(tables, arg);
        arg = va_arg(args, GHashTable*);
    }
    va_end(args);
    // we sort ascending and reverse
    // this makes it so that every intersect_s is comparing
    // the smallest lists possible
    tables = g_list_sort(tables, (GCompareFunc) hash_size_cmp);
    return set_intersect(tables);
}

GHashTable *set_new (GHashFunc hash_func, GEqualFunc equal_func, GDestroyNotify destroy)
{
      return g_hash_table_new_full (hash_func, equal_func, destroy, NULL);
}

void set_add (GHashTable *set, gpointer element)
{
      g_hash_table_replace (set, element, element);
}

gboolean set_contains (GHashTable *set, gpointer element)
{
      return g_hash_table_lookup_extended (set, element, NULL, NULL);
}

gboolean set_remove (GHashTable *set, gpointer element)
{
      return g_hash_table_remove (set, element);
}
