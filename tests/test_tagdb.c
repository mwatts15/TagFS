/*
   Don't change the tests in main.
   They are meant to be comprehensive tests of TagDB
   they can be commented out for other uses.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "util.h"
#include "tagdb.h"
#include "tagdb_priv.h"
#include "types.h"
#include "test_util.h"

void print_pair_hash_value (gpointer key, gpointer val, gpointer not_used)
{
    if (val == NULL)
    {
        val = "null";
    }
    printf("%p=>", key);
    print_hash(val);
}

void print_hash_tree (GHashTable *hsh)
{
    printf("{");
    if (hsh != NULL)
        g_hash_table_foreach(hsh, print_pair_hash_value, NULL);
    printf("}");
    printf("\n");
}

unsigned short rand_lim (unsigned short limit) {
/* return a random number between 0 and limit inclusive.
 */
    int divisor = RAND_MAX/(limit+1);
    unsigned short retval;

    do { 
        retval = rand() / divisor;
    } while (retval > limit);

    return retval;
}

char *spc_rand_ascii(char *buf, size_t len)
{
    char *p = buf;

    while (--len)
        *p++ = (char) ('a' + rand_lim(26));
    *p = 0;
    return buf;
}

GList *generate_test_inputs (int n)
{
    GList *res = NULL;
    int i;
    char *filename = NULL;
    for (i = 0; i < n; i++)
    {
        filename = calloc(20, sizeof(char));
        spc_rand_ascii(filename, 20);
        res = g_list_prepend(res, filename);
    }
    return res;
}

void test_list (TagDB *db, gulong *tags)
{
    GList *f = get_files_list(db, tags);
    GList *t = get_tags_list(db, tags);
    
    print_list(f, file_to_string); g_list_free(f);
    print_list(t, file_to_string); g_list_free(t);
}

void test_db(TagDB *db)
{
    print_hash(db->tags);

    printf("one\n");
    gulong tags[] = {0, 0};
    test_list(db, tags);

    printf("two\n");
    gulong tags2[] = {2, 6, 0};
    test_list(db, tags2);
    
    printf("three\n");
    gulong tags3[] = {2, 6, 7, 8, 9, 0};
    test_list(db, tags3);

    printf("four\n");
    test_list(db, NULL);
    printf("DONE\n");
}

int main ()
{
    srand(time(NULL));
    TagDB *db = tagdb_load("test.db");

    int i;
    for (i = 0; i < 1; i++)
    {
        tagdb_save(db, "saved.db");
        printf("test iteration : %d\n", i);
        test_db(db);
        //db = tagdb_load("saved.db");
    }
    tagdb_destroy(db);
    return 0;
}
