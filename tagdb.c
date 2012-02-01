#include "tagdb.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

GNode *_insert_tag (GNode *tree, const char *str)
{
    char **tags;
    GNode *cur;
    GNode *cat;
    // chomp leading /
    if (g_str_has_prefix(str, "/"))
    {
        str++;
    }
    tags = g_strsplit(str, "/", -1);
    cur = tree;
    int i = 0;
    while (tags[i] != NULL)
    {
        cat = g_node_first_child(cur);
        while (cat != NULL)
        {
            // == 0 -> strings match
            if (g_strcmp0(cat->data, tags[i]) == 0)
            {
                break;
            }
            cat = g_node_next_sibling(cat);
        }
        if (cat == NULL)
        {
            cat = g_node_append_data(cur, tags[i]);
        }
        cur = cat;
        i++;
    }
    return tree;
}

GNode *_tagstruct_from_file (const char *tag_fname)
{
    FILE *tag_file = fopen(tag_fname, "r");
    if (tag_file == NULL)
    {
        perror("Error opening file");
    }
    GNode *res = g_node_new("%ROOT%");
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
            res = _insert_tag(res, g_string_free(accu, FALSE));
            accu = g_string_new("");
        }
        c = fgetc(tag_file);
    }
    fclose(tag_file);
    return res;
}

GHashTable *_string_to_file_tag_struct (const char *str)
{
    GHashTable *res = g_hash_table_new(NULL, g_str_equal);
    char **tags =  g_strsplit(str, ",", -1);
    int i = 0;
    char **tag_val_pair;
    while (tags[i] != NULL)
    {
        tag_val_pair = g_strsplit(tags[i], ":", 2);
        g_hash_table_insert(res, tag_val_pair[0], tag_val_pair[1]);
        i++;
    }
    g_strfreev(tags);
    return res;
}

GHashTable *_dbstruct_from_file (const char *db_fname)
{
    GHashTable *res = g_hash_table_new(NULL, g_str_equal);
    FILE *db_file = fopen(db_fname, "r");
    if (db_file == NULL)
    {
        perror("Error opening file");
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
            key = g_string_free(accu, FALSE);
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
                    g_hash_table_insert(res, key,
                            _string_to_file_tag_struct(g_string_free(accu, 
                                    FALSE)));
                    c = fgetc(db_file);
                    break;
                }
            }
        }
    }
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

GNode *tagdb_toTagTree (tagdb *db)
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

// Return the field value if the field and item exists
// returns NULL if either the item doesn't exist or
// the field isn't associated with the item
gpointer tagdb_get (tagdb *db, const char *item, const char *field)
{
    // Gets really into the database hashtable structure
    // basically db->hash->[HashTable]
    //                     |item|->tags->[HashTable]
    //                                     |field|->value
    //                                     |field|->value
    //                     |item|->tags->...
    GHashTable *hsh = tagdb_toHash(db);
    if (g_hash_table_lookup(hsh, item) != NULL)
    {
        hsh = g_hash_table_lookup(hsh, item);
        return g_hash_table_lookup(hsh, field);
    }
    return NULL;
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
            res = g_list_append(res, key);
        }
    }
    return res;
}

// data holds the current path string
void _get_tag_list (GNode *tree, gchar *path, GList **result)
{
    // add path + "/" + tree->data to result
    (*result) = g_list_append((*result), g_strconcat(path, "/", tree->data, 
                NULL));
    if (g_node_first_child(tree) != NULL)
    {
        _get_tag_list(g_node_first_child(tree), 
                g_strconcat(path, "/", tree->data, NULL), result);
    }
    if (g_node_next_sibling(tree) != NULL)
    {
        _get_tag_list(g_node_next_sibling(tree), path, result);
    }
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
    GList *result = NULL;
    // We just skip the root node. A bit awkward, but whatevs
    _get_tag_list(g_node_first_child(tagdb_toTagTree(db)), "", &result);
    return result;
}

gboolean has_tag_filter (gpointer key, gpointer value, gpointer data)
{
    // Should be a hash, not list
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
        tags = g_list_append(tags, tag);
        tag = va_arg(args, char*);
    }
    va_end(args);
    return tagdb_filter(db, has_tag_filter, tags);
}

void insert_tag (tagdb *db, const char *tag)
{
    db->tagstruct = _insert_tag(db->tagstruct, tag);
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
    g_hash_table_insert(tags, tag, val);
    return db_struct;
}

void insert_file_tag (tagdb *db, const char *filename, const char *tag)
{
    _insert_file_tag(db->dbstruct, filename, tag, "");
}
