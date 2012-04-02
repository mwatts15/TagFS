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
    char *filename;
    for (i = 0; i < n; i++)
    {
        filename = calloc(20, sizeof(char));
        spc_rand_ascii(filename, 20);
        res = g_list_prepend(res, filename);
    }
    return res;
}

void test_insert_files_with_tags (tagdb *db, GList *files, GList *tags)
{
    GList *itf = files;
    GList *itt = tags;
    GHashTable *tag_table;

    while (itf != NULL && itt != NULL)
    {
        tag_table = g_hash_table_new(g_direct_hash, g_direct_equal);
        GList *ittt = itt;
        while (ittt != NULL)
        {
            set_add(tag_table, ittt->data);
            ittt = ittt->next;
        }
        printf("inserting %p into %s with %p\n", itf->data, "file table", tag_table);
        tagdb_insert_item(db, NULL, tag_table, FILE_TABLE);
        itf = itf->next;
        itt = itt->next;
    }
}

void test_insert_files (tagdb *db, GList *files)
{
    GList *it = files;
    while (it != NULL)
    {
        printf("inserting %d into %s\n", GPOINTER_TO_INT(it->data), "file table");
        tagdb_insert_item(db, NULL, g_hash_table_new(g_direct_hash, g_direct_equal), FILE_TABLE);
        it = it->next;
    }
}

void test_insert_tags (tagdb *db, GList *tags)
{
    GList *it = tags;
    while (it != NULL)
    {
        printf("inserting %s into %s\n", (char*) it->data, "tag table");
        tagdb_insert_item(db, it->data, NULL, TAG_TABLE);
        it = it->next;
    }
}

/*
   inserts random files and tags
 */
void test_inserts (tagdb *db, int n)
{
    printf("Testing insertions ...\n");
    GList *files = generate_test_inputs(n);
    GList *tags = generate_test_inputs(n);
    GList *it = files;
    test_insert_files(db, files);
    test_insert_tags(db, tags);
    test_insert_files_with_tags(db, files, tags);
    printf("\n");
}

void test_removes (tagdb *db, int n)
{
    printf("Testing removals ...\n");
    int i;
    int selected;
    for (i = 0; i < n; i++)
    {
        selected = rand_lim(n);
        printf("removing %d from %s\n", selected, "file table");
        tagdb_remove_item(db, selected, FILE_TABLE);
    }
    for (i = 0; i < n; i++)
    {
        selected = rand_lim(n);
        printf("removing %d from %s\n", selected, "tag table");
        tagdb_remove_item(db, selected, TAG_TABLE);
    }
    printf("Testing sub-removals ...\n");
    for (i = 0; i < n; i++)
    {
        selected = rand_lim(n);
        printf("removing %d from %d in %s\n", selected, i, "tag table");
        tagdb_remove_sub(db, i, selected, TAG_TABLE);
    }
    for (i = 0; i < n; i++)
    {
        selected = rand_lim(n);
        printf("removing %d from %d in %s\n", selected, i, "file table");
        tagdb_remove_sub(db, i, selected, FILE_TABLE);
    }
    printf("\n");
}

// for each file in db
// for each tag in file
// verify tag[file] != NULL
void verify_parity (tagdb *db)
{
    printf("Verifying table parity ...\n");
    GHashTable *files = tagdb_files(db);
    GHashTableIter it, itt;
    gpointer key, value, k, v;
    g_hash_table_iter_init(&it, files);
    while (g_hash_table_iter_next(&it, &key, &value))
    {
        GHashTable *tags = (GHashTable*) value;
        g_hash_table_iter_init(&itt, tags);
        while (g_hash_table_iter_next(&itt, &k, &v))
        {
            if (tagdb_get_sub(db, GPOINTER_TO_INT(k), GPOINTER_TO_INT(key), TAG_TABLE) == NULL)
            {
                printf("TagDB does NOT have parity\n");
                return;
            }
        }
    }
    printf("TagDB does have parity\n");
    printf("\n");
}

void test_lookups (tagdb *db)
{
    char *table = "FILE";
    tagdb_query(
}

int main ()
{
    srand(time(NULL));
    tagdb *db = newdb("test.db");
    print_hash_tree(db->tables[0]);
    print_hash_tree(db->tables[1]);
    verify_parity(db);
    test_inserts(db, 10);
    verify_parity(db);
    tagdb_save(db, "saved.db");
    test_removes(db, 10);
    verify_parity(db);
    //print_hash_tree(db->tables[FILE_TABLE]);
    //print_hash_tree(db->tables[TAG_TABLE]);
    test_lookups(db);
    verify_parity(db);
    test_lookups(db);
    return 0;
}
