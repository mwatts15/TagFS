#include "util.h"
#include <stdlib.h>

unsigned short rand_lim (unsigned short limit) {
/* return a random number between 0 and limit inclusive.
 */
    int divisor = RAND_MAX/(limit+1);
    unsigned short retval;

    do { 
        retval = rand() / divisor;
    } while (retval > limit);

    return retval + 1;
}

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

GHashTable *intersect_s (GHashTable *a, GHashTable *b)
{
    GHashTable *tmp;

    if (g_hash_table_size(a) > g_hash_table_size(b))
    {
        tmp = a;
        a = b;
        b = tmp;
    }
    return _intersect_s(a, b);
}

// The keys are the sizezs of the tables.
GHashTable *_intersect_p (GList *tables)
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
    return intersect_s(tables->data, _intersect_p(g_list_next(tables)));
}

GHashTable *intersect_p (GHashTable *a, ...)
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
    tables = g_list_reverse(g_list_sort(tables,
                (GCompareFunc) hash_size_cmp));
    return _intersect_p(tables);
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        srand(atoi(argv[1]));
        argc--;
        argv++;
    }
    
    long max;
    if (argc > 1)
    {
        max = atol(argv[1]);
        argc--;
        argv++;
    }
    else
    {
        max = 50;
    }
    
    long ntables;
    if (argc > 1)
    {
        ntables = atol(argv[1]);
    }
    else
    {
        ntables = 13;
    }

    int i;
    int j;
    int r;
    int table_size;
    GHashTable *table;
    GList *tables = NULL;
    for (i = 0; i < ntables; i++)
    {
        table = g_hash_table_new(g_direct_hash, g_direct_equal);
        table_size = rand_lim(max);
        for (j = 0; j < table_size; j++)
        {
            r = rand_lim(max);
            g_hash_table_insert(table, GINT_TO_POINTER(r), GINT_TO_POINTER(r));
        }
        tables = g_list_prepend(tables, table);
    }
    GHashTable *res = _intersect_p(tables);
    //print_hash(res);
    return 0;
}
