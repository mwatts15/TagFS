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
    return g_hash_table_remove(db->forward, fname);
}

int tagdb_insert_file (tagdb *db, const char *fname)
{
    // might need to worry about unreffing file tag structs
    g_hash_table_insert(db->forward, (gpointer) fname, 
            (gpointer) g_hash_table_new(g_str_hash, g_str_equal));
    return 0;
}

void insert_tag (tagdb *db, const char *tag)
{
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
// 4 separate data structures are read in for the db file
// Two are hash tables for doing most of our accesses
//
// The other two are balanced binary trees (glib standard)
// which keep keys of the two hash tables sorted on their
// values.
// 
// The forward tree can be built from reading the forward database
// The reverse tree can as well, however this requires a lot more
// operations for keeping track of the files under a tag and sorting
// them.
// instead, we only build the reverse tree if we have a reverse
// database stored from a previous mount
void _dbstruct_from_file (tagdb *db, const char *db_fname)
{
    FILE *db_file = fopen(db_fname, "r");
    if (db_file == NULL)
    {
        perror("Error opening file");
        exit(1);
    }

    GHashTable *forward = g_hash_table_new(g_str_hash, g_str_equal);
    GHashTable *reverse = g_hash_table_new(g_str_hash, g_str_equal);
    CodeTable *file_codes = code_table_new();
    CodeTable *tag_codes = code_table_new();
    GHashTable *file_tags = NULL;
    GHashTable *tag_files = NULL;
    char c;
    GString *accu;
    char *file;
    char *tag;
    char *value;

    accu = g_string_new("");
    while (!feof(db_file))
    {
        c = fgetc(db_file);
        if (c != ' ')
        {
            accu = g_string_append_c(accu, c);
        }
        else // state 2 : tags
        {
            file = g_strdup(accu->str);
            g_string_free(accu, TRUE);
            file_tags = g_hash_table_new(g_str_hash, g_str_equal);
            // add an entry into the code table
            code_table_new_entry(file_codes, file);
            g_hash_table_insert(forward, file, file_tags);
            accu = g_string_new("");
            while (!feof(db_file))
            {
                c = fgetc(db_file);
                // Accumulate until we see a separator
                if (c == ':')
                {
                    tag = g_strdup(accu->str);
                    g_string_free(accu, TRUE);
                    accu = g_string_new("");
                    // just in case. this seems to be a problem.
                    value = NULL;
                }
                else if (c == ',' || c == ' ') // last tag
                {
                    value = g_strdup(accu->str);
                    g_string_free(accu, TRUE);
                    accu = g_string_new("");
                    // lookup in code table
                    if (g_hash_table_lookup(reverse, tag) == NULL)
                    {
                        code_table_new_entry(tag_codes, tag);
                        tag_files = g_hash_table_new(g_str_hash,
                                g_str_equal);
                        g_hash_table_insert(reverse, tag, tag_files);
                    }
                    else
                    {
                        tag_files = g_hash_table_lookup(reverse, tag);
                    }
                    g_hash_table_insert(tag_files, file, value);
                    g_hash_table_insert(file_tags, tag, value);
                    if (c == ' ')
                    {
                        break;
                    }
                }
                else
                {
                    accu = g_string_append_c(accu, c);
                }
            }
        }
    }
    if (accu != NULL)
    {
        g_string_free(accu, TRUE);
    }
    fclose(db_file);
    db->forward = forward;
    db->reverse = reverse;
    db->file_codes = file_codes;
    db->tag_codes = tag_codes;
}

tagdb *newdb (const char *db_fname)
{
    tagdb *db = malloc(sizeof(tagdb));
    db->db_fname = db_fname;
    _dbstruct_from_file(db, db_fname);
    return db;
}

GHashTable *tagdb_toHash (tagdb *db)
{
    return db->forward;
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
     db->forward = _insert_file_tag(db->forward, filename, tag, tag);
     insert_tag(db, tag);
}
