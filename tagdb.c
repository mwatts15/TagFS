#include "tagdb.h"
#include <stdlib.h>
#include <stdio.h>

tagdb *newdb (const char *db_fname, const char *tags_fname)
{
    tagdb *db = malloc(sizeof(tagdb));
    db->tag_list_fname = tags_fname;
    db->db_fname = db_fname;
    // open the db file

    // read the records into the dbstruct
    // it's organized like:
    // filename tag1:value,tag2:value,tag3:value
    //   a lot of the tags won't have a value
    
    // open the tag list file
    // read the tags into tagstruct
    // each line is like:
    // parent_tag/child_tag1/child_tag2
    // close the file

    // return the new db
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
void _get_tag_list (GNode *tree, gchar *path, GList *result)
{
    printf("%s\n", tree->data);
    // add path + "/" + tree->data to result
    result = g_list_append(result, g_strconcat(path, "/", tree->data, 
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
    printf("result = ");
    print_list(result);
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
    _get_tag_list(tagdb_toTagTree(db), "", result);
    return result;
}

gboolean has_tag_filter (gpointer key, gpointer value, gpointer data)
{
    GList *file_tags = value;
    GList *query_tags = data;
    while (query_tags != NULL)
    {
        if (g_list_find(file_tags, query_tags->data) == NULL)
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
    GList *tags;
    va_list args;
    va_start(args, db);

    char *tag = va_arg(args, char*);
    while (tag != NULL)
    {
        tags = g_list_append(tags, tag);
        tag = va_arg(args, char*);
    }
    return tagdb_filter(db, has_tag_filter, tags);
}
