#include <stdlib.h>
#include "util.h"
#include "set_ops.h"
#include "test_util.h"

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
    char *resfile = "hash_sets_out";
    char *verifile = "hash_sets_ver";
    FILE *out = fopen(resfile, "w");
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
        max = 20;
    }
    
    long ntables;
    if (argc > 1)
    {
        ntables = atol(argv[1]);
    }
    else
    {
        ntables = 3;
    }

    int i;
    int j;
    int k;
    int r;
    int table_size;
    GHashTable *table = NULL;
    for (k = 0; k < 10; k++)
    {
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
            fprint_hash(out, table);
            tables = g_list_append(tables, table);
        }
        fprintf(out, "intersection: ");
        GHashTable *res = set_intersect(tables);
        fprint_hash(out, res);
        fprintf(out, "union: ");
        res = set_union(tables);
        fprint_hash(out, res);
        fprintf(out, "difference: ");
        res = set_difference(tables);
        fprint_hash(out, res);
    }
    fclose(out);
    print_result("Intersect, union, difference", resfile, verifile);
    return 0;
}
