#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "tagdb.h"
#include "tagdb_priv.h"
#include "tokenizer.h"
#include "util.h"
#include "types.h"
#include "log.h"
#include "set_ops.h"

/* Reads in the types file
   You'll want to call this before dbstruct_from_file
   so that you can handle different tag data than strings */
void tags_from_file (TagDB *db, Tokenizer *tok)
{
    _log_level = 1;
    //log_msg("Entering tag_types_from_file\n");
    char *sep;
    char *token = tokenizer_next(tok, &sep);
    long ntags = atol(token);
    g_free(token);
    
    int i;
    for (i = 0; i < ntags; i++)
    {
        char *id_str = tokenizer_next(tok, &sep);
        char *tagname = tokenizer_next(tok, &sep);
        char *type_str = tokenizer_next(tok, &sep);
        char *default_value = tokenizer_next(tok, &sep);

        if (strlen(tagname) != 0)
        {
            int type = atoi(type_str);
            Tag *t = new_tag(tagname, type, NULL);
            t->default_value = tagdb_str_to_value(type, default_value);
            t->id = atol(id_str);
            insert_tag(db, t);
            if (t->id > db->file_max_id)
            {
                db->tag_max_id = t->id;
            }

        }
        g_free(id_str);
        g_free(tagname);
        g_free(type_str);
        g_free(default_value);
    }
}

void tags_to_file (TagDB *db, FILE *f)
{
    fprintf(f, "%ld", tagdb_ntags(db));
    putc('\0', f);

    GHashTableIter it;
    gpointer k, v;
    g_hash_loop(db->tags, it, k, v)
    {
        Tag *t = (Tag*) v;
        fprintf(f, "%ld", t->id);
        putc('\0', f);
        fprintf(f, "%s", t->name);
        putc('\0', f);
        fprintf(f, "%d", t->type);
        putc('\0', f);

        char *defval = tagdb_value_to_str(t->default_value);
        fprintf(f, "%s", defval);
        putc('\0', f);

        g_free(defval);
    }
}

/* 
   The database file is formated like this:
   the first three null-separatated chunks give the offsets
   for the main database, the meta database, and the types database
   <id_number>NULL<number of tag/value pairs>NULL<NULL-separated tag/value pairs>NULL<next record>
 */
void files_from_file (TagDB *db, Tokenizer *tok)
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
    db->nfiles = nitems;

    gulong i;
    for (i = 0; i < nitems; i++)
    {
        char *token = tokenizer_next(tok, &sep);
        //printf("item_id=%s\n", token);

        gulong item_id = atol(token);
        g_free(token);

        if (item_id == 0)
        {
            fprintf(stderr, "Got item_id == 0 in dbstruct_from_file\n");
            exit(1);
        }

        if (item_id > db->file_max_id)
        {
            db->file_max_id = item_id;
        }

        token = tokenizer_next(tok, &sep);

        File *f = new_file(token);
        f->id = item_id;

        g_free(token);

        token = tokenizer_next(tok, &sep);
        // get the tag count
        int ntags = atoi(token);
        
        g_free(token);
        //printf("number of tags: %d\n", ntags);

        printf("file = %s\n", f->name);
        int j;
        for (j = 0; j < ntags; j++)
        {
            token = tokenizer_next(tok, &sep);
            Tag *t = retrieve_tag(db, atol(token));

            if (t == NULL)
            {
                fprintf(stderr, "Invalid tag %s, skipping\n", token);
                g_free(token);
                token = tokenizer_next(tok, &sep);
            }
            else
            {
                char *tag_value = tokenizer_next(tok, &sep);
                tagdb_value_t *val = NULL;
                if (tag_value != NULL)
                    val = tagdb_str_to_value(t->type, tag_value);
                add_tag_to_file(db, f, t->name, val);
                g_free(tag_value);
            }
            g_free(token);
        }
        insert_file(db, f);
    }
    g_free(nitems_str);
}

void files_to_file (TagDB *db, FILE *f)
{
    GList *tags = g_hash_table_get_keys(db->files);
    GList *res = NULL;
    
    int freeme = 0;
    LL(tags, it)
        GList *slot = g_hash_table_lookup(db->files, it->data);
        slot = g_list_sort(slot, (GCompareFunc) file_id_cmp);
        GList *tmp = g_list_union(res, slot, (GCompareFunc) file_id_cmp);
        if (freeme)
        {
            g_list_free(res);
        }
        res = tmp;
        g_hash_table_insert(db->files, it->data, slot);
        freeme = 1;
    LL_END(it);

    g_list_free(tags);

    fprintf(f, "%ld", (gulong) g_list_length(res));
    putc('\0', f);

    LL(res, list)
        File *fi = (File*) list->data;

        log_msg("fi->name = %s\n", fi->name);
        fprintf(f, "%ld", fi->id);
        putc('\0', f);
        fprintf(f, "%s", fi->name);
        putc('\0', f);

        GHashTable *tags = fi->tags;
        if (tags && g_hash_table_size(tags))
        {
            fprintf(f, "%d", g_hash_table_size(tags));
            putc('\0', f);
            HL(tags, it, k, v)
                char *value = NULL;
                value = tagdb_value_to_str((tagdb_value_t*) v);
                fprintf(f, "%ld", TO_S(k));
                putc('\0', f);
                fprintf(f, "%s", value);
                putc('\0', f);

                g_free(value);
            HL_END;
        }
        else
        {
            putc('0', f);
            putc('\0', f);
        }
    LL_END(list);
    g_list_free(res);
}

