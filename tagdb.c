#include "tagdb.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

GList *_insert_tag (GList *tags, const char *tag)
{
    GList *cur = tags;
    while (cur != NULL)
    {
        if (g_strcmp0(tag, cur->data) == 0)
        {
            return tags;
        }
        cur = g_list_next(cur);
    }
    return g_list_insert_sorted(tags, (gpointer) tag, (GCompareFunc) g_strcmp0);
}

int tagdb_remove_file (tagdb *db, const char *fname)
{
    return g_hash_table_remove(db->dbstruct, fname);
}

int tagdb_insert_file (tagdb *db, const char *fname)
{
    // might need to worry about unreffing file tag structs
    g_hash_table_insert(db->dbstruct, (gpointer) fname, 
            (gpointer) g_hash_table_new(g_str_hash, g_str_equal));
    return 0;
}

void insert_tag (tagdb *db, const char *tag)
{
    db->tagstruct = _insert_tag(db->tagstruct, tag);
}

GList *_tagstruct_from_file (const char *tag_fname)
{
    FILE *tag_file = fopen(tag_fname, "r");
    if (tag_file == NULL)
    {
        perror("Error opening file");
    }
    GList *res = NULL;
    GString *accu = g_string_new("");
    char c = fgetc(tag_file);
    while (!feof(tag_file))
    {
        if (c != ' ')
        {
            accu = g_string_append_c(accu, c);
        }
        else
        {
            res = _insert_tag(res, (const char*) g_strdup(accu->str));
            g_string_free(accu, TRUE);
            accu = g_string_new("");
        }
        c = fgetc(tag_file);
    }
    if (accu != NULL)
        g_string_free(accu, TRUE);
    fclose(tag_file);
    return res;
}

GHashTable *_string_to_file_tag_struct (const char *str)
{
    GHashTable *res = g_hash_table_new(g_str_hash, g_str_equal);
    char **tags =  g_strsplit(str, ",", -1);
    int i = 0;
    char **tag_val_pair;
    while (tags[i] != NULL)
    {
        tag_val_pair = g_strsplit(tags[i], ":", 2);
        g_hash_table_insert(res, strdup(tag_val_pair[0]), 
                strdup(tag_val_pair[1]));
        g_strfreev(tag_val_pair);
        i++;
    }
    g_strfreev(tags);
    return res;
}

// Takes two tag hashe tables
// and compares based on tag name then on tag value
// -1 is the first is greater, 0 is both are the same, 1 is the second is greater
int file_tags_cmp (GHashTable *atags, GHashTable *btags)
{
    /*
    GList *akeys = g_hash_table_get_keys(atags);
    GList *bkeys = g_hash_table_get_keys(btags);
    */
    GList *akeys = g_list_sort(g_hash_table_get_keys(atags), (GCompareFunc) g_strcmp0);
    GList *bkeys = g_list_sort(g_hash_table_get_keys(btags), (GCompareFunc) g_strcmp0);
    int res = 0;
    while (akeys != NULL && bkeys != NULL)
    {
        printf("%s and %s\n", (char*) akeys->data, (char*) bkeys->data);
        res = g_strcmp0(akeys->data, bkeys->data);
        if (res != 0)
        {
            g_list_free(akeys);
            g_list_free(bkeys);
            return res;
        }
        akeys = akeys->next;
        bkeys = bkeys->next;
    }
    if (akeys == NULL && bkeys != NULL)
    {
        g_list_free(akeys);
        g_list_free(bkeys);
        return -1;
    }
    if (bkeys == NULL && akeys != NULL)
    {
        g_list_free(akeys);
        g_list_free(bkeys);
        return 1;
    }
    g_list_free(akeys);
    g_list_free(bkeys);
    return 0;
}

// Reads in the db file
// Assumes the entries are already sorted, so they had better be
GHashTable *_dbstruct_from_file (const char *db_fname)
{
    GHashTable *res = g_hash_table_new(g_str_hash, g_str_equal);
    GHashTable *tags;
    FILE *db_file = fopen(db_fname, "r");
    if (db_file == NULL)
    {
        perror("Error opening file");
        exit(1);
    }

    char c = fgetc(db_file);
    GString *accu = g_string_new("");
    char *key;
    while (!feof(db_file))
    {
        // state 1: file name
        if (c != ' ')
        {
            accu = g_string_append_c(accu, c);
            c = fgetc(db_file);
        }
        else // state 2 : tags
        {
            key = g_strdup(accu->str);
            g_string_free(accu, TRUE);
            c = fgetc(db_file);
            accu = g_string_new("");
            while (!feof(db_file))
            {
                if (c != ' ')
                {
                    accu = g_string_append_c(accu, c);
                    c = fgetc(db_file);
                }
                else // add the filename tags pair to the dbstruct and goto state 1
                {
                    tags = _string_to_file_tag_struct(g_strdup(accu->str));
                    g_string_free(accu, TRUE);
                    accu = g_string_new("");
                    g_hash_table_insert(res, key, tags);
                    c = fgetc(db_file);
                    break;
                }
            }
        }
    }
    if (accu != NULL)
    {
        g_string_free(accu, TRUE);
    }
    tags = NULL;
    fclose(db_file);
    return res;
}

tagdb *newdb (const char *db_fname, const char *tags_fname)
{
    tagdb *db = malloc(sizeof(tagdb));
    db->tag_list_fname = tags_fname;
    db->db_fname = db_fname;
    db->dbstruct = _dbstruct_from_file(db_fname);
    db->tagstruct = _tagstruct_from_file(tags_fname);
    return db;
}

GHashTable *tagdb_toHash (tagdb *db)
{
    return db->dbstruct;
}

GList *tagdb_tagstruct (tagdb *db)
{
    return db->tagstruct;
}

GList *tagdb_files (tagdb *db)
{
    return g_hash_table_get_keys(tagdb_toHash(db));
}

// Return all of the fields of item as a hash
// returns NULL if item cannot be found
// never returns NULL when item is in db
GHashTable *tagdb_get_file_tags (tagdb *db, const char *item)
{
    return g_hash_table_lookup(tagdb_toHash(db), item);
}

GList *tagdb_get_file_tag_list (tagdb *db, const char *item)
{
    return g_hash_table_get_keys(g_hash_table_lookup(tagdb_toHash(db), 
                item));
}

// returns NULL if either the item doesn't exist or
gpointer tagdb_get (tagdb *db, const char *item)
{
    // basically db->hash->[HashTable]
    //                     |item|->tags->[HashTable]
    //                                     |field|->value
    //                                     |field|->value
    //                     |item|->tags->...
    GHashTable *hsh = tagdb_toHash(db);
    return g_hash_table_lookup(hsh, item);
}

// returns a list of names of items which satisfy predicate
GList *tagdb_filter (tagdb *db, gboolean (*predicate)(gpointer key,
            gpointer value, gpointer data), gpointer data)
{
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
    return g_list_reverse(res);
}

// get list of all tags
// the hierarchy is compressed like
// parent-->child1->etc.
//       +->child2
// becomes 
// parent/child1/etc.
// parent/child2
// good for printing out to file
GList *get_tag_list (tagdb *db)
{
    return db->tagstruct;
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
    return tagdb_filter(db, has_tag_filter, tags);
}

// Get the file in the db_struct
// Get the tag in the file tag_struct
// insert the tag:val pair
GHashTable *_insert_file_tag (GHashTable *db_struct, const char *filename,
        const char *tag, const char *val)
{
    GHashTable *tags = g_hash_table_lookup(db_struct, filename);
    if (tags == NULL)
    {
        return db_struct;
    }
    g_hash_table_insert(tags, (gpointer) tag, (gpointer) val);
    return db_struct;
}

// Do a binary search based on the tags we're searching for (O(log n))
// run to the first file with those tags (close enough to O(1))
// keep going until we get all of them (O(n) for n files that match)

void tagdb_insert_file_tag (tagdb *db, const char *filename, const char *tag)
{
     db->dbstruct = _insert_file_tag(db->dbstruct, filename, tag, tag);
     insert_tag(db, tag);
}
