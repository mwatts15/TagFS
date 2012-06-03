#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "tagdb.h"
#include "tagdb_priv.h"
#include "set_ops.h"
#include "tokenizer.h"
#include "util.h"
#include "types.h"

// Reads in the types file
// You'll want to call this before dbstruct_from_file
// so that you can handle different tag data than strings
void tag_types_from_file (tagdb *db, Tokenizer *tok)
{
    //log_msg("Entering tag_types_from_file\n");
    char *sep;
    char *token = tokenizer_next(tok, &sep);
    long ntags = atol(token);
    g_free(token);
    
    printf("ntags = %ld\n", ntags);
    int i;
    for (i = 0; i < ntags; i++)
    {
        char *tagname = tokenizer_next(tok, &sep);
        char *type_str = tokenizer_next(tok, &sep);
        printf("%s:%s\n", tagname, type_str);
        if (strlen(tagname) != 0)
        {
            int type = atoi(type_str);
            tagdb_insert_item(db, tagname, NULL, TAG_TABLE);
            tagdb_set_tag_type(db, tagname, type);
        }
        g_free(tagname);
        g_free(type_str);
    }
}

void tag_types_to_file (tagdb *db, FILE *f)
{
    fprintf(f, "%d", g_hash_table_size(db->tag_types));
    putc('\0', f);

    GHashTableIter it;
    gpointer k, v;
    g_hash_loop(db->tag_types, it, k, v)
    {
        fprintf(f, "%s", tagdb_get_tag_value(db, TO_I(k)));
        putc('\0', f);
        fprintf(f, "%d", TO_I(v));
        putc('\0', f);
    }
}

// Reads in the db file
/* 
   The database file is formated like this:
   the first three null-separatated chunks give the offsets
   for the main database, the meta database, and the types database
   <id_number>NULL<number of tag/value pairs>NULL<NULL-separated tag/value pairs>NULL<next record>
 */
void dbstruct_from_file (tagdb *db, int table_id, Tokenizer *tok)
{
    /*
       If keys or values are dynamically allocated, you must be careful to ensure 
       that they are freed when they are removed from the GHashTable, and also when 
       they are overwritten by new insertions into the GHashTable. It is also not 
       advisable to mix static strings and dynamically-allocated strings in a 
       GHashTable, because it then becomes difficult to determine whether the string 
       should be freed. 
     */
    char *sep = NULL;
    char *nitems_str = tokenizer_next(tok, &sep);
    long nitems = atol(nitems_str);
    printf("nitems=%s\n", nitems_str);
    g_free(nitems_str);
    int max_id = 0;

    int i;
    for (i = 0; i < nitems; i++)
    {
        char *token = tokenizer_next(tok, &sep);
        printf("item_id=%s\n", token);

        int item_id = 0;
        if (table_id == FILE_TABLE)
            item_id = atoi(token);
        else
            item_id = tagdb_get_tag_code(db, token);

        g_free(token);

        if (item_id == 0)
        {
            fprintf(stderr, "Got item_id == 0 in dbstruct_from_file\n");
            exit(1);
        }

        if (item_id > max_id)
        {
            max_id = item_id;
        }

        // get the tag count
        token = tokenizer_next(tok, &sep);
        printf("number of tags: %s\n", token);
        int ntags = atoi(token); g_free(token);

        int j;
        for (j = 0; j < ntags; j++)
        {
            token = tokenizer_next(tok, &sep);
            printf("tag name : %s\n", token);
            int tag_type = tagdb_get_tag_type(db, token);
            int tag_code = tagdb_get_tag_code(db, token);
            g_free(token);

            if (tag_code == 0)
            {
                fprintf(stderr, "Must have tags in code table before reading db\n");
                exit(1);
            }

            token = tokenizer_next(tok, &sep);
            tagdb_value_t *val = tagdb_str_to_value(tag_type, token);
            g_free(token);

            tagdb_insert_sub(db, item_id, tag_code, val, table_id);
        }
    }
    if (table_id == FILE_TABLE)
        db->last_id = max_id;
}

void dbstruct_to_file (tagdb *db, int table_id, FILE *f)
{
    fprintf(f, "%d", g_hash_table_size(db->tables[table_id]));
    putc('\0', f);

    GHashTableIter it,itt;
    gpointer key, value, k, v;
    g_hash_loop(db->tables[table_id], it, key, value)
    {
        if (table_id == FILE_TABLE)
            fprintf(f, "%d", GPOINTER_TO_INT(key));
        else
            fprintf(f, "%s", tagdb_get_tag_value(db, key));
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

        g_hash_loop(tags, itt, k, v)
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
    g_hash_table_insert(sub_table, GINT_TO_POINTER(new_id), new_data);
}

void _insert_item (tagdb *db, int item_id,
        GHashTable *data, int table_id)
{
    g_hash_table_insert(db->tables[table_id], GINT_TO_POINTER(item_id), data);

    if (table_id == META_TABLE)
        return;
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
