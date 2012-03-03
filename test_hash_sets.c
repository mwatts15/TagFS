#include "util.h"
#include "hash_ops.h"
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
    GHashTable *res = intersect(tables);
    //print_hash(res);
    return 0;
}
