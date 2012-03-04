/*
   Don't change the tests in main.
   They are meant to be comprehensive tests of tagdb
   they can be commented out for other uses.
 */
#include "tagdb.h"
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include <time.h>

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
    g_hash_table_foreach(hsh, print_pair_hash_value, NULL);
    printf("}");
    printf("\n");
}

void test_insert_files (tagdb *db, GList *files)
{
    GList *it = files;
    while (it != NULL)
    {
        tagdb_insert_item(db, GPOINTER_TO_INT(it->data), FILE_TABLE);
        it = it->next;
    }
}
void test_insert_files_with_tags (tagdb *db, GList *files, GList *tags)
{
    GList *itf = files;
    GList *itt = tags;
    while (itf != NULL && itt != NULL)
    {
        tagdb_insert_file_with_tags(db, itf->data, itt);
        itf = itf->next;
        itt = itt->next;
    }
}
void test_insert_tags (tagdb *db, GList *tags)
{
    GList *it = tags;
    while (it != NULL)
    {
        tagdb_insert_item(db, GPOINTER_TO_INT(it->data), TAG_TABLE);
        it = it->next;
    }
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

GList *generate_test_inputs ()
{
    GList *res = NULL;
    int i;
    char *filename;
    for (i = 0; i < 2000; i++)
    {
        filename = calloc(20, sizeof(char));
        spc_rand_ascii(filename, 20);
        res = g_list_prepend(res, filename);
    }
    return res;
}
/*
   inserts random files and tags
 */
void test_inserts (tagdb *db)
{
    GList *files = generate_test_inputs();
    GList *tags = generate_test_inputs();
    GList *it = files;
    test_insert_files(db, files);
    test_insert_tags(db, tags);
    test_insert_files_with_tags(db, files, tags);
}

void test_removes (tagdb *db, int n)
{
    int i;
    for (i = 0; i < n; i++)
    {
        tagdb_remove_item(db, rand_lim(n), FILE_TABLE);
    }
    for (i = 0; i < n; i++)
    {
        tagdb_remove_item(db, rand_lim(n), TAG_TABLE);
    }
}

void test_lookups (tagdb *db)
{
}

int main ()
{
    srand(time(NULL));
    tagdb *db = newdb("test.db");
    print_hash(db->tables[0]);
    print_hash(db->tables[1]);
    test_inserts(db);
    test_removes(db, 50);
    print_hash(db->tables[FILE_TABLE]);
    print_hash(db->tables[TAG_TABLE]);
    test_lookups(db);
    return 0;
}
