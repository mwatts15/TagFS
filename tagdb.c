#include "tagdb.h"
#include "tokenizer.h"
#include "set_ops.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// Reads in the db file
// 4 separate data structures are read in for the db file
// Two are hash tables for doing most of our accesses
// The other two are code translation tables for the tags
// and for the files
void _dbstruct_from_file (tagdb *db, const char *db_fname)
{
    GList *seps = g_list_new_charlist(' ', ',', ':', 0);

    Tokenizer *tok = tokenizer_new(seps);
    if (tokenizer_set_file_stream(tok, db_fname) < 0)
    {
        exit(1);
    }

    int file_id;
    int its_code;

    GHashTable *forward = g_hash_table_new(g_direct_hash, g_direct_equal);
    GHashTable *reverse = g_hash_table_new(g_direct_hash, g_direct_equal);
    GHashTable *its_tags = g_hash_table_new(g_direct_hash, g_direct_equal);
    CodeTable *tag_codes = code_table_new();

    char sep;
    char *token = tokenizer_next(tok, &sep);
    file_id = 0;
    its_code = 0;
    while (token != NULL)
    {
        if (sep == ':')
        {
            // store the tag name and get its code
            its_code = code_table_ins_entry(tag_codes, token);
            g_free(token);
        }
        if (sep == ' ' | sep == ',' | sep == -1) // -1 is signals the end of the file
        {
            // store the file for this tag
            GHashTable *its_files = g_hash_table_lookup(reverse, GINT_TO_POINTER(its_code));
            if (its_files == NULL)
            {
                its_files = set_new(g_direct_hash, g_direct_equal, NULL);
                g_hash_table_insert(reverse, GINT_TO_POINTER(its_code), its_files);
            }
            set_add(its_files, GINT_TO_POINTER(file_id));
            // store the tag/value pair for this file
            g_hash_table_insert(its_tags, GINT_TO_POINTER(its_code), token);
            if (sep == ' ' | sep == -1)
            {
                // store this new file into forward with its tags
                g_hash_table_insert(forward, GINT_TO_POINTER(file_id), its_tags);
                file_id++;
            }
            if (sep == ' ')
            {
                // make a new hash for the next file
                its_tags = g_hash_table_new(g_direct_hash, g_direct_equal);
            }
        }
        token = tokenizer_next(tok, &sep);
    }
    tokenizer_destroy(tok);
    g_free(token);
    db->tables[0] = forward;
    db->tables[1] = reverse;
    db->tag_codes = tag_codes;
}

tagdb *newdb (const char *db_fname)
{
    tagdb *db = malloc(sizeof(tagdb));
    db->db_fname = db_fname;
    db->tables = calloc(2, sizeof(GHashTable*));
    _dbstruct_from_file(db, db_fname);
    return db;
}

// gets list of file ids for some query
// or gets list of tag ids for some query
// or gets some string for some query
// etc.

GHashTable *tagdb_get_sub (tagdb *db, int item_id, int sub_id, int table_id)
{
    GHashTable *sub_table = tagdb_get_item(db, item_id, table_id);
    return g_hash_table_lookup(sub_table, GINT_TO_POINTER(sub_id));
}

void tagdb_remove_sub (tagdb *db, int item_id, int sub_id, int table_id)
{
    GHashTable *sub_table = tagdb_get_item(db, item_id, table_id);
    if (sub_table != NULL)
    {
        g_hash_table_remove(sub_table, GINT_TO_POINTER(item_id));
    }
}

// tags is a list of tag IDs
GHashTable *get_files_by_tag_list (tagdb *db, GList *tags)
{
    GList *file_tables = NULL;
    while (tags != NULL)
    {
        file_tables = g_list_prepend(file_tables, 
                tagdb_get_item(db, GPOINTER_TO_INT(tags->data), TAG_TABLE));
        tags = tags->next;
    }
    return set_intersect(file_tables);
}

void tagdb_remove_item (tagdb *db, int item_id, int table_id)
{
    GHashTable *its_associates = tagdb_get_item(db, item_id, table_id);
    if (its_associates == NULL)
    {
        return;
    }

    GHashTableIter it;
    gpointer key, value;
    g_hash_table_iter_init(&it, its_associates);
    int other = (table_id == FILE_TABLE)?TAG_TABLE:FILE_TABLE;
    if (g_hash_table_iter_next(&it, &key, &value))
    {
        printf("removing %d from %s\n", GPOINTER_TO_INT(key), (other == FILE_TABLE)?"File table":"Tag table");
        tagdb_remove_sub(db, item_id, GPOINTER_TO_INT(key), other);
    }
    g_hash_table_remove(db->tables[table_id], GINT_TO_POINTER(item_id));
}

GHashTable *tagdb_get_item (tagdb *db, int item_id, int table_id)
{
    return g_hash_table_lookup(db->tables[table_id], GINT_TO_POINTER(item_id));
}

tagdb_insert_file_with_tags(tagdb *db, const char *file, GList *tags)
{
}

void tagdb_insert_item (tagdb *db, int item_id, int table_id)
{
//    g_hash_table_insert(db->tables[table_id], GINT_TO_POINTER(item_id));
}

void tagdb_insert_sub (tagdb *db, int item_id, int sub_id, int table_id)
{
//    GHashTable *sub_table = tagdb_get_item(db, item_id, table_id);
//    g_hash_table_insert(sub_table, GINT_TO_POINTER(sub_id));
}

GHashTable *tagdb_get_table(tagdb *db, int table_id)
{
    return db->tables[table_id];
}

/*
// returns a list of names of items which satisfy predicate
GList *tagdb_filter (tagdb *db, gboolean (*predicate)(gpointer key,
            gpointer value, gpointer data), gpointer data)
{
    if (data == NULL)
    {
        return tagdb_files(db);
    }
    GList *res = NULL;
    GHashTableIter it;
    gpointer key, value;
    g_hash_table_iter_init(&it, tagdb_toHash(db));
    while (g_hash_table_iter_next(&it, &key, &value))
    {
        if (predicate(key, value, data))
        {
            res = g_list_prepend(res, key);
        }
    }
    return res;
}

gboolean has_tag_filter (gpointer key, gpointer value, gpointer data)
{
    GHashTable *file_tags = value;
    GList *query_tags = data;
    while (query_tags != NULL)
    {
        if (g_hash_table_lookup(file_tags, query_tags->data) == NULL)
        {
            return FALSE;
        }
        query_tags = g_list_next(query_tags);
    }
    return TRUE;
}

// get files with tags
// tag list must end with NULL
GList *get_files_by_tags (tagdb *db, ...)
{
    // I just copy the args into a GList. it's easier this way.
    GList *tags = NULL;
    va_list args;
    va_start(args, db);

    char *tag = va_arg(args, char*);
    while (tag != NULL)
    {
        tags = g_list_prepend(tags, tag);
        tag = va_arg(args, char*);
    }
    tags = g_list_reverse(tags);
    va_end(args);
    return get_files_by_tag_list(db, tags);
}

GList *get_files_by_tag_list (tagdb *db, GList *tags)
{
    GList *file_tables = NULL;
    while (tags != NULL)
    {
        file_tables = g_list_append(file_tables, tagdb_get_tag_files(db, tags->data));
        tags = tags->next;
    }
    GList *tmp = g_hash_table_get_keys(intersect(file_tables));
    GList *res = NULL;
    GList *it = tmp;
    while (it != NULL)
    {
        int code = GPOINTER_TO_INT(it->data);
        char *fname = code_table_get_value(db->file_codes, code);
        res = g_list_prepend(res, fname);
        it = it->next;
    }
    g_list_free(tmp);
    return res;
}
*/
