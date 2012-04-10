/*
   Don't change the tests in main.
   They are meant to be comprehensive tests of tagdb
   they can be commented out for other uses.
 */
#include "tagdb.h"
#include "query.h"
#include "params.h" // added for fuse in log.c
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
    GList *file_it = files;
    GList *tag_it = tags;
    GHashTable *tag_table;
    result_t *val;
    int tcode;

    while (file_it != NULL && tag_it != NULL)
    {
        tag_table = g_hash_table_new(g_direct_hash, g_direct_equal);
        GList *file_tag_list_it = tag_it;
        while (file_tag_list_it != NULL)
        {
            tcode = tagdb_get_tag_code(db, file_tag_list_it->data);
            val = encapsulate(tagdb_int_t, TO_P(rand_lim(1024)));
            g_hash_table_insert(tag_table, TO_P(tcode), &(val->data));
            file_tag_list_it = file_tag_list_it->next;
        }
        printf("inserting %p into %s with %p\n", file_it->data, "file table", tag_table);
        tagdb_insert_item(db, NULL, tag_table, FILE_TABLE);
        file_it = file_it->next;
        tag_it = tag_it->next;
    }
}

void test_insert_files (tagdb *db, GList *files)
{
    union tagdb_value *val;
    GList *it = files;
    int ncode = tagdb_get_tag_code(db, "name");
    while (it != NULL)
    {
        GHashTable *hash = g_hash_table_new(g_direct_hash, g_direct_equal);
        val = tagdb_str_to_value(tagdb_str_t, it->data);
        g_hash_table_insert(hash, GINT_TO_POINTER(ncode), val);
        printf("inserting file with name %s into %s\n", (char*) it->data, "file table");
        tagdb_insert_item(db, NULL, hash, FILE_TABLE);
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
        tagdb_set_tag_type(db, it->data, tagdb_int_t);
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
    printf("Verifying table parity...\n");
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

void test_db(tagdb *db)
{
    print_hash(db->tag_types);
    printf("File table:\n");
    print_hash_tree(db->tables[FILE_TABLE]);
    printf("Tag table:\n");
    print_hash_tree(db->tables[TAG_TABLE]);
    verify_parity(db);
    test_inserts(db, 10);
    verify_parity(db);
    test_removes(db, 10);
    verify_parity(db);
    tagdb_save(db, "saved.db", "saved.types");
    printf("DONE\n");
}

int main ()
{
    srand(time(NULL));
    tagdb *db = newdb("test.db", "test.types");
    test_db(db);
    db = newdb("saved.db", "saved.types");
    test_db(db);
    return 0;
}
