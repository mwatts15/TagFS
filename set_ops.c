#include "util.h"
#include "set_ops.h"
#include <stdlib.h>

// can't return 0, else same size hashses
// would get overwritten
gint hash_size_cmp (GHashTable *a, GHashTable *b)
{
    int asize;
    int bsize;

    if (a == NULL)
        asize = 0;
    else
        asize = g_hash_table_size((GHashTable*) a);

    if (b == NULL)
        bsize = 0;
    else
        bsize = g_hash_table_size((GHashTable*) b);
    
    return asize - bsize;
}

// returns NULL if neither set is NULL, else returns
// a valid result based on arguments
// the correct result of any operation on two null sets
// is a null (empty) set
GHashTable *null_set_check (GHashTable *a, GHashTable *b, 
        gpointer if_a_null, gpointer if_b_null)
{
    if (a == NULL && b == NULL)
    {
        return set_new(g_direct_hash, g_direct_equal, NULL);
    }
    if (a == NULL)
    {
        return if_a_null;
    }
    if (b == NULL)
    {
        return if_b_null;
    }
    return NULL;
}

// assumes a is the smaller
GHashTable *_intersect_s (GHashTable *a, GHashTable *b)
{
    GHashTable *empty_hash = g_hash_table_new(g_direct_hash, g_direct_equal);
    GHashTable *checked = null_set_check(a, b, empty_hash, empty_hash);
    if (checked != NULL) // Means one or both is NULL
    {
        return checked;
    }
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

GHashTable *_union_s (GHashTable *a, GHashTable *b)
{
    GHashTable *checked = null_set_check(a, b, b, a);
    if (checked != NULL)
    {
        return checked;
    }
    GHashTableIter it;
    gpointer key;
    gpointer value;
    GHashTable *res = g_hash_table_new(g_direct_hash, g_direct_equal);

    g_hash_table_iter_init(&it, b);
    while (g_hash_table_iter_next(&it, &key, &value))
    {
        g_hash_table_insert(res, key, value);
    }
    g_hash_table_iter_init(&it, a);
    while (g_hash_table_iter_next(&it, &key, &value))
    {
        if (g_hash_table_lookup(b, key) == NULL)
        {
            g_hash_table_insert(res, key, value);
        }
    }
    return res;

}

// puts sets in order of size
GHashTable *set_intersect_s (GHashTable *a, GHashTable *b)
{
    if (hash_size_cmp(a, b) < 0)
        return _intersect_s(a, b);
    else
        return _intersect_s(b, a);
}

GHashTable *set_union_s (GHashTable *a, GHashTable *b)
{
    if (hash_size_cmp(a, b) < 0)
        return _union_s(a, b);
    else
        return _union_s(b, a);
}

GHashTable *set_union (GList *sets)
{
    GHashTable *res = NULL;
    GHashTable *tmp = NULL;
    while (sets != NULL)
    {
        tmp = set_union_s(sets->data, res);
        res = tmp;
        sets = sets->next;
    }
    return res;
}

GHashTable *set_intersect (GList *tables)
{
    if (tables == NULL)
    {
        return NULL;
    }
    if (g_list_next(tables) == NULL)
    {
        return tables->data;
    }
    GHashTable *res = tables->data;
    GHashTable *tmp = NULL;
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

GHashTable *set_difference_s (GHashTable *a, GHashTable *b)
{
    GHashTable *checked = null_set_check(a, b, a, a);
    if (checked != NULL)
    {
        return checked;
    }
    GHashTableIter it;
    gpointer key;
    gpointer value;
    GHashTable *res = g_hash_table_new(g_direct_hash, g_direct_equal);

    g_hash_table_iter_init(&it, a);
    while (g_hash_table_iter_next(&it, &key, &value))
    {
        if (g_hash_table_lookup(b, key) == NULL)
        {
            g_hash_table_insert(res, key, value);
        }
    }
    return res;
}

// does first - second - third - ...
GHashTable *set_difference (GList *sets)
{
    if (sets == NULL)
    {
        return NULL;
    }
    GHashTable *res = sets->data;
    sets = sets->next;
    GHashTable *tmp = NULL;
    while (sets != NULL)
    {
        tmp = set_difference_s(res, sets->data);
        res = tmp;
        sets = sets->next;
    }
    return res;
}

GHashTable *set_subset (GHashTable *hash, set_predicate pred, gpointer user_data)
{
    GHashTableIter it;
    gpointer k, v;
    GHashTable *res = g_hash_table_new(g_direct_hash, g_direct_equal);

    g_hash_loop(hash, it, k, v)
    {
        if (pred(k, v, user_data))
        {
            g_hash_table_insert(res, k, v);
        }
    }
    return res;
}

GHashTable *set_new (GHashFunc hash_func, GEqualFunc equal_func, GDestroyNotify destroy)
{
      return g_hash_table_new_full (hash_func, equal_func, destroy, NULL);
}

void set_add (GHashTable *set, gpointer element)
{
      g_hash_table_replace (set, element, element);
}

gboolean set_equal_s (GHashTable *a, GHashTable *b)
{
    if (hash_size_cmp(a, b) == 0)
        return _equal_s(a, b);
    else
        return FALSE;
}

gboolean _equal_s (GHashTable *a, GHashTable *b)
{
    if (a == NULL) // means a and b are null. null==null
    {
        return TRUE;
    }
    GHashTableIter it;
    gpointer key;
    gpointer value;

    g_hash_table_iter_init(&it, a);
    while (g_hash_table_iter_next(&it, &key, &value))
    {
        if (g_hash_table_lookup(b, key) == NULL)
            return FALSE;
    }
    return TRUE;
}

gboolean set_contains (GHashTable *set, gpointer element)
{
      return g_hash_table_lookup_extended (set, element, NULL, NULL);
}

gboolean set_remove (GHashTable *set, gpointer element)
{
      return g_hash_table_remove (set, element);
}
