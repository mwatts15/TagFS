#include <stdio.h>
#include <stdlib.h>
#include "tagdb.h"
#include "set_ops.h"
#include "tokenizer.h"
#include "util.h"
#include "types.h"

// Reads in the types file
// You'll want to call this before dbstruct_from_file
// so that you can handle different tag data than strings
void tag_types_from_file (tagdb *db, const char *types_fname)
{
    GList *seps = g_list_new_charlist(':', '\n', NULL);
    db->tag_types = g_hash_table_new(g_direct_hash, g_direct_equal);

    Tokenizer *tok = tokenizer_new(seps);
    if (tokenizer_set_file_stream(tok, types_fname) < 0)
    {
        exit(2);
    }
    char sep;
    char *tagname = tokenizer_next(tok, &sep);
    char *value = tokenizer_next(tok, &sep);
    while (tagname != NULL && value != NULL)
    {
        int tcode;
        int type;
        tcode = code_table_ins_entry(db->tag_codes, tagname);
        type = atoi(value);
        //printf("%d:%d\n", tcode, type);
        g_hash_table_insert(db->tag_types, GINT_TO_POINTER(tcode), GINT_TO_POINTER(type));
        tagname = tokenizer_next(tok, &sep);
        value = tokenizer_next(tok, &sep);
        //g_free(tagname);
    }
}

// Reads in the db file
// 4 separate data structures are read in for the db file
// Two are hash tables for doing most of our accesses
// The other two are code translation tables for the tags
// and for the files
void dbstruct_from_file (tagdb *db, const char *db_fname)
{
    GList *seps = g_list_new_charlist('|', ' ', ',', ':', NULL);

    Tokenizer *tok = tokenizer_new(seps);
    if (tokenizer_set_file_stream(tok, db_fname) < 0)
    {
        exit(1);
    }

    int file_id;
    int tag_code;
    int max_id;
    union tagdb_value *val;

    GHashTable *forward = g_hash_table_new(g_direct_hash, g_direct_equal);
    GHashTable *reverse = g_hash_table_new(g_direct_hash, g_direct_equal);
    GHashTable *its_tags = g_hash_table_new(g_direct_hash, g_direct_equal);

    char sep;
    char *token = tokenizer_next(tok, &sep);
    file_id = 0;
    tag_code = 0;
    max_id = file_id;
    while (token != NULL)
    {
        if (sep == '|')
        {
            // convert the token to a number and set as the file_id
            file_id = atoi(token);
            if (file_id == 0)
            {
                fprintf(stderr, "Got file_id == 0 in _dbstruct_from_file\n");
                exit(1);
            }
            if (file_id > max_id)
            {
                max_id = file_id;
            }
            g_free(token);
        }
        if (sep == ':')
        {
            // signifies no tags for the file
            // we skip this by simply getting the next token
            // storing the file
            if (g_strcmp0(token, "*") == 0)
            {
                tag_code = code_table_ins_entry(db->tag_codes, "UNTAGGED");
            }
            else
            {
                // store the tag name and get its code
                tag_code = code_table_ins_entry(db->tag_codes, token);
            }
            g_free(token);
        }
        if (sep == ' ' | sep == ',' | sep == -1) // -1 is signals the end of the file
        {
            if (tag_code == 0)
            {
                fprintf(stderr, "Got tag_code==0 in _dbstruct_from_file\n");
                exit(1);
            }
            // store the file for this tag
            GHashTable *its_files = g_hash_table_lookup(reverse, GINT_TO_POINTER(tag_code));
            if (its_files == NULL)
            {
                its_files = g_hash_table_new(g_direct_hash, g_direct_equal);
                g_hash_table_insert(reverse, GINT_TO_POINTER(tag_code), its_files);
            }
            // store the tag/value pair for this file
            int tag_type = tagdb_get_tag_type_from_code(db, tag_code);
            val = tagdb_str_to_value(tag_type, token);
            g_hash_table_insert(its_tags, GINT_TO_POINTER(tag_code), val);
            g_hash_table_insert(its_files, GINT_TO_POINTER(file_id), val);
            if (sep == ' ' | sep == -1)
            {
                // store this new file into forward with its tags
                g_hash_table_insert(forward, GINT_TO_POINTER(file_id), its_tags);
            }
            if (sep == ' ')
            {
                // make a new hash for the next file
                tag_code = 0;
                its_tags = g_hash_table_new(g_direct_hash, g_direct_equal);
            }
            // for error-checking
        }
        token = tokenizer_next(tok, &sep);
    }
    tokenizer_destroy(tok);
    g_free(token);
    db->last_id = max_id;
    db->tables[0] = forward;
    db->tables[1] = reverse;
}

void _remove_sub (tagdb *db, int item_id, int sub_id, int table_id)
{
    GHashTable *sub_table = tagdb_get_item(db, item_id, table_id);
    if (sub_table != NULL)
    {
        g_hash_table_remove(sub_table, GINT_TO_POINTER(sub_id));
    }
}

void _insert_sub (tagdb *db, int item_id, int new_id, 
        gpointer new_data, int table_id)
{
    GHashTable *sub_table = tagdb_get_item(db, item_id, table_id);
    if (sub_table == NULL)
    {
        sub_table = g_hash_table_new(g_direct_hash, g_direct_equal);
        g_hash_table_insert(db->tables[table_id], GINT_TO_POINTER(item_id),
                sub_table);
    }
    g_hash_table_insert(sub_table, GINT_TO_POINTER(new_id), new_data);
}

void _insert_item (tagdb *db, int item_id,
        GHashTable *data, int table_id)
{
    // inserts do not overwrite sub tables
    // although data is a hash table, that is only used as an interface
    // for collections with data
    // **you should not assume that the values in data will be the
    //   ones in the resulting hash table**
    GHashTable *orig_table = tagdb_get_item(db, item_id, table_id);
    g_hash_table_insert(db->tables[table_id], GINT_TO_POINTER(item_id),
            set_union_s(data, orig_table));
    GHashTable *sub_table = tagdb_get_item(db, item_id, table_id);

    GHashTableIter it;
    gpointer key, value;
    g_hash_table_iter_init(&it, sub_table);

    int other = (table_id == FILE_TABLE)?TAG_TABLE:FILE_TABLE;
    while (g_hash_table_iter_next(&it, &key, &value))
    {
        // check if we have it, don't create it
        // this assumes that if we are inserting a tag with a
        // bunch of files (like a move directory or something)
        // the values being inserted are default values or something
        // like that.
        _insert_sub(db, GPOINTER_TO_INT(key), item_id, value, other);
    }
}
