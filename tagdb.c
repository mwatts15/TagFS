#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "tagdb.h"
#include "tokenizer.h"
#include "set_ops.h"
#include "util.h"
#include "query.h"
#include "types.h"
#include "code_table.h"

void tagdb_save (tagdb *db, const char *db_fname, const char *tag_types_fname)
{
    dbstruct_to_file(db, db_fname);
    tag_types_to_file(db, tag_types_fname);
}

tagdb *newdb (const char *db_fname, const char *tag_types_fname)
{
    tagdb *db = malloc(sizeof(struct tagdb));
    char cwd[PATH_MAX];
    getcwd(cwd, PATH_MAX);
    db->db_fname = g_strdup_printf("%s/%s", cwd, db_fname);
    db->types_fname = g_strdup_printf("%s/%s", cwd, tag_types_fname);
    printf("db name: %s\ntypes name: %s\n", db->db_fname, 
            db->types_fname);

    db->tables = calloc(2, sizeof(GHashTable*));

    db->tag_codes = code_table_new();
    tag_types_from_file(db, db->types_fname);
    dbstruct_from_file(db, db->db_fname);
    //print_hash(db->tables[0]);
    //print_hash(db->tables[1]);
    return db;
}

// does all of the steps in query.c for you
result_t *tagdb_query (tagdb *db, const char *query)
{
    gpointer r;
    query_t *q = parse(query);
    int type = -1;
    act(db, q, &r, &type);
    query_destroy(q);
    return encapsulate(type, r);
}

gpointer tagdb_get_sub (tagdb *db, int item_id, int sub_id, int table_id)
{
    GHashTable *sub_table = tagdb_get_item(db, item_id, table_id);
    if (sub_table != NULL)
    {
        return g_hash_table_lookup(sub_table, GINT_TO_POINTER(sub_id));
    }
    else
    {
        fprintf(stderr, "ERROR, tagdb_get_sub, tagdb_get_item returns NULL\n \
                item_id = %d, sub_id = %d, table_id = %d\n", item_id, sub_id, table_id);
        return NULL;
    }
}

// a corresponding removal should be done in the other table 
// we can't do it here because recursion's a bitch, and structural
// modifications to hash tables aren't allowed.
// so we do it here
void tagdb_remove_sub (tagdb *db, int item_id, int sub_id, int table_id)
{
    int other = (table_id == FILE_TABLE)?TAG_TABLE:FILE_TABLE;
    _remove_sub(db, item_id, sub_id, table_id);
    _remove_sub(db, sub_id, item_id, other);
}

void tagdb_remove_item (tagdb *db, int item_id, int table_id)
{
    GHashTable *sub_table = tagdb_get_item(db, item_id, table_id);
    if (sub_table == NULL)
    {
        return;
    }

    GHashTableIter it;
    gpointer key, value;
    g_hash_table_iter_init(&it, sub_table);
    int other = (table_id == FILE_TABLE)?TAG_TABLE:FILE_TABLE;
    while (g_hash_table_iter_next(&it, &key, &value))
    {
        _remove_sub(db, GPOINTER_TO_INT(key), item_id, other);
    }
    g_hash_table_remove(db->tables[table_id], GINT_TO_POINTER(item_id));
}

GHashTable *tagdb_get_item (tagdb *db, int item_id, int table_id)
{
    //log_msg("%p\n", db->tables[table_id]);
    return g_hash_table_lookup(db->tables[table_id], GINT_TO_POINTER(item_id));
}

// if item_id == -1 and it's a file then we give it a new_id of last_id + 1
// by convention a 'filename' gets inserted with the "name" tag, but this isn't
// required
// item should have NULL for inserting a file, but it's ignored anyaway
int tagdb_insert_item (tagdb *db, gpointer item,
        GHashTable *data, int table_id)
{
    if (data == NULL)
        data = g_hash_table_new(g_direct_hash, g_direct_equal);
    if (table_id == FILE_TABLE)
    {
        int file_id = TO_I(item);
        if (file_id == 0)
        {
            db->last_id++;
            file_id = db->last_id;
        }
        if (TO_I(item) > db->last_id)
            db->last_id = TO_I(item);
        _insert_item(db, file_id, data, table_id);
        return db->last_id;
    }
    if (table_id == TAG_TABLE)
    {
        // item should not be NULL, but a string
        if (item == NULL)
        {
            fprintf(stderr, "tagdb_insert_item with table_id %d should \
                    not have item==NULL\n", table_id);
        }
        int code = code_table_ins_entry(db->tag_codes, (char*) item);
        _insert_item(db, code, data, table_id);
        return code;
    }
    fprintf(stderr, "tagdb_insert_item, invalid table_id");
    return 0;
}

// new_data may be NULL for tag table
// NOTE: this is what you should use if you want to change the
//   tags for a file
void tagdb_insert_sub (tagdb *db, int item_id, int new_id, 
        gpointer new_data, int table_id)
{
    int other = (table_id == FILE_TABLE)?TAG_TABLE:FILE_TABLE;
    _insert_sub(db, item_id, new_id, new_data, table_id);
    _insert_sub(db, new_id, item_id, new_data, other);
}

GHashTable *tagdb_files (tagdb *db)
{
    return tagdb_get_table(db, FILE_TABLE);
}

GHashTable *tagdb_get_table (tagdb *db, int table_id)
{
    return db->tables[table_id];
}

/*
GHashTable *tagdb_get_files_by_tag_value (tagdb *db, const char *tag, gpointer value, 
        GCompareFunc cmp, int inclusion_condition)
{
    GHashTable *res = g_hash_table_new(g_direct_hash, g_direct_equal);
    // look up tag in tag table
    GHashTable *files = tagdb_get_item(db, tag, TAG_TABLE);
    // filter those files by equality with value
    GHashTableIter it;
    gpointer k, v, not_used;
    g_hash_table_iter_init(&it, files);
    while (g_hash_table_iter_next(&it, &k, &not_used))
    {
        v = tagdb_get_sub(db, GPOINTER_TO_INT(k), tag, FILE_TABLE);
        if (cmp(value, v) == inclusion_condition)
        {
            g_hash_table_insert(res, k, v);
        }
    }
    return res;
}
*/

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

char *tagdb_get_tag_value (tagdb *db, int code)
{
    return code_table_get_value(db->tag_codes, code);
}

int tagdb_get_tag_code (tagdb *db, const char *tag_name)
{
    return code_table_get_code(db->tag_codes, tag_name);
}

int tagdb_get_tag_type_from_code (tagdb *db, int code)
{
    return GPOINTER_TO_INT(g_hash_table_lookup(db->tag_types, GINT_TO_POINTER(code)));
}

int tagdb_get_tag_type (tagdb *db, const char *tag_name)
{
    int tcode = tagdb_get_tag_code(db, tag_name);
    return tagdb_get_tag_type_from_code(db, tcode);
}

void tagdb_set_tag_type (tagdb *db, const char *tag_name, int type)
{
    int tcode = tagdb_get_tag_code(db, tag_name);
    tagdb_set_tag_type_from_code(db, tcode, type);
}

void tagdb_set_tag_type_from_code (tagdb *db, int tag_code, int type)
{
    g_hash_table_insert(db->tag_types, GINT_TO_POINTER(tag_code), GINT_TO_POINTER(type));
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
*/
