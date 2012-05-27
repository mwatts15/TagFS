#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "tagdb.h"
#include "tagdb_priv.h"
#include "set_ops.h"
#include "tokenizer.h"
#include "util.h"
#include "types.h"

// Reads in the types file
// You'll want to call this before dbstruct_from_file
// so that you can handle different tag data than strings
void tag_types_from_file (tagdb *db, const char *types_fname)
{
    log_msg("Entering tag_types_from_file\n");
    GList *seps = g_list_new(":", "\n", NULL);
    db->tag_types = g_hash_table_new(g_direct_hash, g_direct_equal);

    Tokenizer *tok = tokenizer_new(seps);
    if (tokenizer_set_file_stream(tok, types_fname) < 0)
    {
        exit(2);
    }

    char *sep;
    char *tagname;
    char *type_str;
    int tcode;
    int type;
    while (!tokenizer_stream_is_empty(tok->stream))
    {
        tagname = tokenizer_next(tok, &sep);
        type_str = tokenizer_next(tok, &sep);
        tcode = code_table_ins_entry(db->tag_codes, tagname);
        type = atoi(type_str);
        //printf("%d:%d\n", tcode, type);
        g_hash_table_insert(db->tag_types, GINT_TO_POINTER(tcode), GINT_TO_POINTER(type));
        g_free(tagname);
        g_free(type_str);
    }
    tokenizer_destroy(tok);
}

void tag_types_to_file (tagdb *db, const char* filename)
{
    if (filename == NULL)
    {
        filename = db->types_fname;
    }
    FILE *f = fopen(filename, "w");
    if (f == NULL)
    {
        exit(1);
    }
    GHashTableIter it;
    gpointer k, v;
    g_hash_loop(db->tag_types, it, k, v)
    {
        fprintf(f, "%s:%d\n", tagdb_get_tag_value(db, TO_I(k)), TO_I(v));
    }
    fprintf(f, "\n");
    fclose(f);
}

// Reads in the db file
/* 
   The database file is formated like this:
   <id_number>NULL<number of tag/value pairs>NULL<NULL-separated tag/value pairs>NULL<next record>
 */
void dbstruct_from_file (tagdb *db, const char *db_fname)
{
    log_msg("Entering dbstruct_from_file\n");
    GList *seps = g_list_new("\0", NULL);
    Tokenizer *tok = tokenizer_new(seps);

    if (tokenizer_set_file_stream(tok, db_fname) < 0)
    {
        exit(1);
    }

    /*
       If keys or values are dynamically allocated, you must be careful to ensure 
       that they are freed when they are removed from the GHashTable, and also when 
       they are overwritten by new insertions into the GHashTable. It is also not 
       advisable to mix static strings and dynamically-allocated strings in a 
       GHashTable, because it then becomes difficult to determine whether the string 
       should be freed. 
     */
    db->tables[FILE_TABLE] = g_hash_table_new_full(g_direct_hash, g_direct_equal,
            NULL, (GDestroyNotify) g_hash_table_destroy);
    db->tables[TAG_TABLE] = g_hash_table_new_full(g_direct_hash, g_direct_equal, 
            NULL, (GDestroyNotify) g_hash_table_destroy);

    char *sep = NULL;
    char *token = NULL;
    int file_id = 0;
    int tag_code = 0;
    int max_id = 0;

    while (!tokenizer_stream_is_empty(tok->stream))
    {
        // get the id
        token = tokenizer_next(tok, &sep);
        printf("id: %s\n", token);
        file_id = atoi(token); g_free(token);

        if (file_id == 0)
        {
            fprintf(stderr, "Got file_id == 0 in dbstruct_from_file\n");
            exit(1);
        }

        if (file_id > max_id)
        {
            max_id = file_id;
        }

        // get the tag count
        token = tokenizer_next(tok, &sep);
        //printf("number of tags: %s\n", token);
        int ntags = atoi(token); g_free(token);

        int i;
        for (i = 0; i < ntags; i++)
        {
            token = tokenizer_next(tok, &sep);
            int tag_type = tagdb_get_tag_type(db, token);
            //printf("%s::%s = ", token, type_strings[tag_type]);
            int tag_code = tagdb_get_tag_code(db, token);
            g_free(token);

            if (tag_code == 0)
            {
                fprintf(stderr, "Must have tags in code table before reading db\n");
                exit(1);
            }

            token = tokenizer_next(tok, &sep);
            log_msg("%s\n", token);
            tagdb_value_t *val = tagdb_str_to_value(tag_type, token);
            g_free(token);

            tagdb_insert_sub(db, file_id, tag_code, val, FILE_TABLE);
        }
    }
    tokenizer_destroy(tok);
    db->last_id = max_id;
}

void dbstruct_to_file (tagdb *db, const char *filename)
{
    if (filename == NULL)
    {
        filename = db->db_fname;
    }
    FILE *f = fopen(filename, "w");
    if (f == NULL)
    {
        exit(1);
    }
    GHashTableIter it,itt;
    gpointer key, value, k, v;
    g_hash_table_iter_init(&it, db->tables[FILE_TABLE]);
    while (g_hash_table_iter_next(&it, &key, &value))
    {
        // print the id
        //log_msg("%d\n", key);
        fprintf(f, "%d", GPOINTER_TO_INT(key));
        putc('\0', f);
        //log_msg("SAVING....\n");
        GHashTable *tags = tagdb_get_item(db, GPOINTER_TO_INT(key), 
                FILE_TABLE);
        if (tags!=NULL) 
            fprintf(f, "%d", g_hash_table_size(tags));
        else
            putc('0', f);
        putc('\0', f);
        if (tags == NULL || g_hash_table_size(tags) == 0)
        {
            continue;
        }
        g_hash_table_iter_init(&itt, tags);
        while (g_hash_table_iter_next(&itt, &k, &v))
        {
            char *str = NULL;
            char *value = NULL;
            str = code_table_get_value(db->tag_codes, GPOINTER_TO_INT(k));
            value = tagdb_value_to_str((tagdb_value_t*) v);
            fprintf(f, "%s", str);
            putc('\0', f);
            fprintf(f, "%s", value);
            putc('\0', f);
            //g_free(value);
        }
    }
    fclose(f);
}

void _remove_sub (tagdb *db, int item_id, int sub_id, int table_id)
{
    GHashTable *sub_table = tagdb_get_item(db, item_id, table_id);
    if (sub_table != NULL)
    {
        g_hash_table_steal(sub_table, GINT_TO_POINTER(sub_id));
    }
}

void _insert_sub (tagdb *db, int item_id, int new_id, 
        gpointer new_data, int table_id)
{
    GHashTable *sub_table = tagdb_get_item(db, item_id, table_id);
    if (sub_table == NULL)
    {
        sub_table = _sub_table_new();
        g_hash_table_insert(db->tables[table_id], GINT_TO_POINTER(item_id),
                sub_table);
    }
    g_hash_table_steal(sub_table, GINT_TO_POINTER(new_id));
    //printf("i = %d\n", i);
    g_hash_table_insert(sub_table, GINT_TO_POINTER(new_id), new_data);
}

void _insert_item (tagdb *db, int item_id,
        GHashTable *data, int table_id)
{
    // inserts do not overwrite sub tables, but unions them.
    g_hash_table_insert(db->tables[table_id], GINT_TO_POINTER(item_id), data);

    GHashTableIter it;
    gpointer key, value;
    g_hash_table_iter_init(&it, data);
    int other = (table_id == FILE_TABLE)?TAG_TABLE:FILE_TABLE;

    // here we insure that the values get updated
    while (g_hash_table_iter_next(&it, &key, &value))
    {
        _insert_sub(db, GPOINTER_TO_INT(key), item_id, value, other);
    }
}

GHashTable *_sub_table_new()
{
        return g_hash_table_new_full(g_direct_hash, g_direct_equal, 
                NULL, NULL);
}
