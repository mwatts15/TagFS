#include "tagdb.h"
#include "tokenizer.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// just removes the file from the code table
// this way any lookups to the code will return
// zero which always indicates that the file
// isn't being tracked.
// actual removal of the file is done by tagfs
int tagdb_remove_file (tagdb *db, const char *fname)
{
    code_table_del_by_value(db->file_codes, fname);
    return 0;
}

int tagdb_remove_tag (tagdb *db, const char *tagname)
{
    code_table_del_by_value(db->tag_codes, tagname);
    return 0;
}

// returns the file's code that isn't
// not really opaque, but it's useful
int tagdb_insert_file (tagdb *db, const char *fname)
{
    int code = code_table_get_code(db->file_codes, fname);
    if (code == 0)
    {
        code = code_table_new_entry(db->file_codes, fname);
        g_hash_table_insert(db->forward, GINT_TO_POINTER(code),
                g_hash_table_new(g_direct_hash, g_direct_equal));
    }
    return code;
}

// inserts a tag without any files attached to it
void tagdb_insert_tag (tagdb *db, const char *tag)
{
    int code = code_table_get_code(db->tag_codes, tag);
    if (code == 0)
    {
        code = code_table_new_entry(db->tag_codes, tag);
        g_hash_table_insert(db->reverse, GINT_TO_POINTER(code), 
                g_hash_table_new(g_direct_hash, g_direct_equal));
    }
}

// Takes two tag hash tables
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
// The other two are code translation tables for the tags
// and for the files
void _dbstruct_from_file (tagdb *db, const char *db_fname)
{
    GHashTable *forward = g_hash_table_new(g_direct_hash, g_direct_equal);
    GHashTable *reverse = g_hash_table_new(g_direct_hash, g_direct_equal);
    CodeTable *file_codes = code_table_new();
    CodeTable *tag_codes = code_table_new();
    GHashTable *file_tags = NULL;
    GHashTable *tag_files = NULL;
    char *file;
    int code;
    char *tag;
    char *value;

    GList *seps;
    seps = g_list_prepend(NULL, GINT_TO_POINTER(' '));
    seps = g_list_prepend(seps, GINT_TO_POINTER(','));
    seps = g_list_prepend(seps, GINT_TO_POINTER(':'));

    Tokenizer *tok = tokenizer_new(seps);
    if (tokenizer_set_file_stream(tok, db_fname) < 0)
    {
        exit(1);
    }

    char sep;
    char *token = tokenizer_next(tok, &sep);

    while (token != NULL)
    {
        if (sep == ' ')
        {
            file = token;
            file_tags = g_hash_table_new(g_direct_hash, g_direct_equal);
            // add an entry into the code table
            code = code_table_new_entry(file_codes, file);
            g_hash_table_insert(forward, GINT_TO_POINTER(code), file_tags);

            token = tokenizer_next(tok, &sep);
            while (token != NULL)
            {
                if (sep == ':')
                {
                    tag = token;
                    if (code_table_get_code(tag_codes, tag) == 0)
                        code_table_new_entry(tag_codes, tag);
                }
                else if (sep == ',' || sep == ' ')
                {
                    value = token;
                    code = code_table_get_code(tag_codes, tag);
                    if (code == 0)
                    {
                        fprintf(stderr, "Malformated database\n");
                        exit(1);
                    }
                    tag_files = g_hash_table_lookup(reverse, GINT_TO_POINTER(code));

                    if (tag_files == NULL)
                    {
                        tag_files = g_hash_table_new(g_direct_hash,
                                g_direct_equal);
                        g_hash_table_insert(reverse, GINT_TO_POINTER(code), tag_files);
                    }

                    g_hash_table_insert(file_tags, GINT_TO_POINTER(code), value);
                    code = code_table_get_code(file_codes, file);
                    g_hash_table_insert(tag_files, GINT_TO_POINTER(code), value);
                    if (sep == ' ')
                    {
                        break;
                    }
                }
                token = tokenizer_next(tok, &sep);
            }
        }
    }
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
    int code = code_table_get_code(db->file_codes, item);
    return g_hash_table_lookup(tagdb_toHash(db), code);
}

GList *tagdb_get_file_tag_list (tagdb *db, const char *item)
{
    return g_hash_table_get_keys(tagdb_get_file_tags(db, item));
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
void _insert_file_tag (GHashTable *db_struct, const char *filename,
        const char *tag, const char *val)
{
    GHashTable *tags = g_hash_table_lookup(db_struct, filename);
    int tcode = code_table_get_code(db->tag_codes, tag);
    if (tcode == 0)
    {
        return db_struct;
    }
    g_hash_table_insert(file_tags, GINT_TO_POINTER(tcode), GINT_TO_POINTER(tcode));
    return db_struct;
}

// adds the file if it doesn't exist already
// adds the given tag to the file's tags and adds
// the file to the tag's files
void tagdb_insert_file_tag (tagdb *db, const char *filename, const char *tag)
{
    int fcode = tagdb_insert_file(db, filename);
    int tcode = code_table_get_code(db->tag_codes, tag);
    GHashTable *file_tags = tagdb_get_file_tags(db, filename);
    g_hash_table_insert(file_tags, GINT_TO_POINTER(tcode), GINT_TO_POINTER(tcode));
    // insert the file there as well
    insert_file_tag(db->forward, filename, tag, tag);
    tagdb_insert_tag(db, tag);
}
