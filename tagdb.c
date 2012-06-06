#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "tagdb.h"
#include "tagdb_priv.h"
#include "tokenizer.h"
#include "set_ops.h"
#include "util.h"
#include "types.h"
#include "code_table.h"

void tagdb_save (tagdb *db, const char *db_fname, const char *tag_types_fname)
{
    if (db_fname == NULL)
    {
        db_fname = db->db_fname;
    }
    FILE *f = fopen(db_fname, "w");
    if (f == NULL)
    {
        log_error("Couldn't open db file for save\n");
    }

    tag_types_to_file(db, f);
    dbstruct_to_file(db, FILE_TABLE, f);
    dbstruct_to_file(db, META_TABLE, f);
    fclose(f);
}

/*
void _seek_to_section (int section, Tokenizer *tok)
{
    tokenizer_seek(tok, 0);

    tokenizer_skip(tok, section);
    char *s;
    char *sectstr = tokenizer_next(tok, &s);
    tokenizer_seek(tok, atol(sectstr));
    g_free(sectstr);
}
*/

int get_other_table_id (int table_id)
{
    int other = table_id;
    if (table_id == FILE_TABLE)
        other = TAG_TABLE;
    else if (table_id == TAG_TABLE)
        other = FILE_TABLE;
    return other;
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
    
    int ntables = 3;
    int i;

    db->tables = calloc(ntables, sizeof(GHashTable*));
    
    for (i = 0; i < ntables; i++)
    {
        db->tables[i] = g_hash_table_new_full(g_direct_hash, g_direct_equal,
            NULL, (GDestroyNotify) g_hash_table_destroy);
    }

    db->tag_codes = code_table_new();
    db->tag_types = g_hash_table_new(g_direct_hash, g_direct_equal);

    const char *seps[] = {"\0", NULL};
    Tokenizer *tok = tokenizer_new_v(seps);
    if (tokenizer_set_file_stream(tok, db_fname) == -1)
    {
        fprintf(stderr, "Couldn't open db file\n");
        return NULL;
    }

    tag_types_from_file(db, tok);
    dbstruct_from_file(db, FILE_TABLE, tok);
    //dbstruct_from_file(db, META_TABLE, tok);
    tokenizer_destroy(tok);
    log_hash(db->tables[META_TABLE]);
    return db;
}

gpointer tagdb_get_sub (tagdb *db, int item_id, int sub_id, int table_id)
{
    GHashTable *sub_table = tagdb_get_item(db, item_id, table_id);
    //print_hash(sub_table);
    if (sub_table != NULL)
    {
        return g_hash_table_lookup(sub_table, GINT_TO_POINTER(sub_id));
    }
    else
    {
        /*fprintf(stderr, "warning: tagdb_get_sub, tagdb_get_item returns NULL\n \
                item_id = %d, sub_id = %d, table_id = %d\n", item_id, sub_id, table_id);
         */
        return NULL;
    }
}

// a corresponding removal should be done in the other table 
// we can't do it here because recursion's a bitch, and structural
// modifications to hash tables aren't allowed.
// so we do it here
void tagdb_remove_sub (tagdb *db, int item_id, int sub_id, int table_id)
{
    result_destroy(tagdb_get_sub(db, item_id, sub_id, table_id));
    _remove_sub(db, item_id, sub_id, table_id);
    if (table_id == FILE_TABLE
            || table_id == TAG_TABLE)
    {
        int other = (table_id == FILE_TABLE)?TAG_TABLE:FILE_TABLE;
        _remove_sub(db, sub_id, item_id, other);
    }
}

void tagdb_remove_item (tagdb *db, int item_id, int table_id)
{
    GHashTable *sub_table = tagdb_get_item(db, item_id, table_id);
    if (sub_table == NULL)
    {
        return;
    }

    if (table_id == FILE_TABLE
            || table_id == TAG_TABLE)
    {
        GHashTableIter it;
        gpointer key, value;
        g_hash_table_iter_init(&it, sub_table);
        int other = (table_id == FILE_TABLE)?TAG_TABLE:FILE_TABLE;
        while (g_hash_table_iter_next(&it, &key, &value))
        {
            _remove_sub(db, GPOINTER_TO_INT(key), item_id, other);
        }
    }
    g_hash_table_remove(db->tables[table_id], GINT_TO_POINTER(item_id));
}

GHashTable *tagdb_get_item (tagdb *db, int item_id, int table_id)
{
    return g_hash_table_lookup(db->tables[table_id], GINT_TO_POINTER(item_id));
}

GList *tagdb_get_file_tag_list (tagdb *db, int item_id)
{
    return g_hash_table_get_keys(tagdb_get_item(db, item_id, FILE_TABLE));
}

// if item_id == -1 and it's a file then we give it a new_id of last_id + 1
// by convention a 'filename' gets inserted with the "name" tag, but this isn't
// required
// item should have NULL for inserting a file, but it's ignored anyaway
// NOTE: This is NOT what you should use if you want to change the
//     tags for a file. Use tagdb_insert_sub() instead.
int tagdb_insert_item (tagdb *db, gpointer item,
        GHashTable *data, int table_id)
{
    if (data == NULL)
        data = _sub_table_new();
    int item_id = 0;
    if (table_id == FILE_TABLE)
    {
        item_id = TO_I(item);
        if (item_id <= 0)
        {
            db->last_id++;
            item_id = db->last_id;
        }
        else
        {
            return 0;
        }
    }
    else if (table_id == TAG_TABLE)
    {
        // item should not be NULL, but a string
        if (item == NULL)
        {
            fprintf(stderr, "tagdb_insert_item with table_id %d should \
                    not have item==NULL\n", table_id);
        }
        item_id = code_table_ins_entry(db->tag_codes, (char*) item);
    }
    else if (table_id == META_TABLE)
    {
        item_id = tagdb_get_tag_code(db, item);
    }

    if (!item_id)
        return 0;

    _insert_item(db, item_id, data, table_id);
    return item_id;
}

// new_data may be NULL for tag table
// NOTE: this is what you should use if you want to change the
//   tags for a file
void tagdb_insert_sub (tagdb *db, int item_id, int sub_id, 
        gpointer data, int table_id)
{
    tagdb_value_t *old_value = tagdb_get_sub(db, item_id, sub_id, table_id);

    // preserve table properties/invariants
    if (old_value == NULL)
    {
        // if the sub entry is already in the table
        // then we assume that the meta table is up-to-date
        // if it isn't, then we need to update it by
        // inserting with _new_sub which creates a new
        // entry if none exists, but does nothing otherwise
        if (table_id == FILE_TABLE || table_id == TAG_TABLE)
        {
            GHashTable *tags = NULL; 
            int tagcode = 0;
            if (table_id == FILE_TABLE)
            {
                tags = tagdb_get_item(db, item_id, FILE_TABLE);
                tagcode = sub_id;
            }
            else
            {
                tags =  tagdb_get_item(db, sub_id, FILE_TABLE);
                tagcode = item_id;
            }

            if (tags != NULL)
            {
                GHashTableIter it;
                gpointer k, v;
                g_hash_loop(tags, it, k, v)
                {
                    result_t *dummy = tagdb_str_to_value(tagdb_get_tag_type_from_code(db, TO_I(k)), "0");
                    _new_sub(db, tagcode, TO_I(k), dummy, META_TABLE);
                    dummy = tagdb_str_to_value(tagdb_get_tag_type_from_code(db, tagcode), "0");
                    _new_sub(db, TO_I(k), tagcode, dummy, META_TABLE);
                }
            }
        }
        else
        {
            GHashTable *files = tagdb_get_item(db, item_id, TAG_TABLE);
            if (files == NULL)
            {
                GHashTableIter it;
                gpointer k, v;
                g_hash_loop(files, it, k, v)
                {
                    _new_sub(db, sub_id, TO_I(k), NULL, FILE_TABLE);
                }
                files = tagdb_get_item(db, sub_id, TAG_TABLE);
                g_hash_loop(files, it, k, v)
                {
                    _new_sub(db, TO_I(k), sub_id, NULL, TAG_TABLE);
                }
            }
        }
    }

    result_destroy(old_value);
    int other = get_other_table_id(table_id);

    _insert_sub(db, sub_id, item_id, data, other);
    _insert_sub(db, item_id, sub_id, data, table_id);
}

GHashTable *tagdb_files (tagdb *db)
{
    return tagdb_get_table(db, FILE_TABLE);
}

gboolean _is_tagged (gpointer k, gpointer v, gpointer not_used)
{
    return (v != NULL && g_hash_table_size(v) > 0);
}

gboolean _is_untagged (gpointer k, gpointer v, gpointer not_used)
{
    return (v == NULL || g_hash_table_size(v) == 0);
}

GHashTable *tagdb_tagged_files (tagdb *db)
{
    return set_subset(tagdb_files(db), _is_tagged, NULL);
}

GHashTable *tagdb_untagged_files (tagdb *db)
{
    return set_subset(tagdb_files(db), _is_untagged, NULL);
}

GHashTable *tagdb_get_table (tagdb *db, int table_id)
{
    return db->tables[table_id];
}

// we dubiously extend the definition of tags
// to include files
GHashTable *tagdb_tagged_items (tagdb *db, int table_id)
{
    return set_subset(tagdb_get_table(db, table_id), _is_tagged, NULL);
}

GHashTable *tagdb_untagged_items (tagdb *db, int table_id)
{
    return set_subset(tagdb_get_table(db, table_id), _is_untagged, NULL);
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

void tagdb_change_tag_name (tagdb *db, char *old_name, char *new_name)
{
    code_table_chg_value(db->tag_codes, old_name, new_name);
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

void tagdb_remove_tag_type (tagdb *db, const char *tag_name)
{
    int tcode = tagdb_get_tag_code(db, tag_name);
    g_hash_table_remove(db->tag_types, GINT_TO_POINTER(tcode));
}

void tagdb_remove_tag_type_from_code (tagdb *db, int tag_code)
{
    g_hash_table_remove(db->tag_types, GINT_TO_POINTER(tag_code));
}
