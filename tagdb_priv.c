#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "tagdb.h"
#include "tagdb_priv.h"
#include "scanner.h"
#include "util.h"
#include "types.h"
#include "log.h"
#include "set_ops.h"

/* Reads in the types file
   You'll want to call this before dbstruct_from_file
   so that you can handle different tag data than strings */
void tags_from_file (TagDB *db, Scanner *scn)
{
    _log_level = 1;
    //log_msg("Entering tag_types_from_file\n");
    char *sep;
    char *token = scanner_next(scn, &sep);
    long ntags = atol(token);
    g_free(token);
    
    int i;
    for (i = 0; i < ntags; i++)
    {
        char *id_str = scanner_next(scn, &sep);
        char *tagname = scanner_next(scn, &sep);
        char *type_str = scanner_next(scn, &sep);
        char *default_value = scanner_next(scn, &sep);

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

        char *defval = NULL;
        if (t->default_value)
        {
            defval = tagdb_value_to_str(t->default_value);
            fprintf(f, "%s", defval);
        }
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
void files_from_file (TagDB *db, Scanner *scn)
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
    char *nitems_str = scanner_next(scn, &sep);
    long nitems = atol(nitems_str);
    db->nfiles = nitems;

    gulong i;
    for (i = 0; i < nitems; i++)
    {
        char *token = scanner_next(scn, &sep);
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

        token = scanner_next(scn, &sep);

        File *f = new_file(token);
        f->id = item_id;

        g_free(token);

        token = scanner_next(scn, &sep);
        // get the tag count
        int ntags = atoi(token);
        
        g_free(token);
        //printf("number of tags: %d\n", ntags);

        printf("file = %s\n", f->name);
        int j;
        for (j = 0; j < ntags; j++)
        {
            token = scanner_next(scn, &sep);
            Tag *t = retrieve_tag(db, atol(token));

            if (t == NULL)
            {
                fprintf(stderr, "Invalid tag %s, skipping\n", token);
                g_free(token);
                token = scanner_next(scn, &sep);
            }
            else
            {
                char *tag_value = scanner_next(scn, &sep);
                tagdb_value_t *val = NULL;
                if (tag_value != NULL)
                    val = tagdb_str_to_value(t->type, tag_value);
                add_tag_to_file(db, f, t->id, val);
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

    /* This is to make sure we pick up any files that were
       overwritten in any of the file slots */
    GList *res = NULL;

    LL(tags, it)
        GList *files = retrieve_file_drawer_l(db, (gulong) it->data);
        files = g_list_sort(files, (GCompareFunc) file_id_cmp);

        GList *tmp = g_list_union(res, files, (GCompareFunc) file_id_cmp);

        g_list_free(res);
        g_list_free(files);
        res = tmp;
        printf("ftf: ");
        print_list(res, file_to_string);
    LL_END(it);

    g_list_free(tags);

    gulong nfiles = g_list_length(res);
    if (nfiles != db->nfiles) log_msg("Warning: file count out of sync\n");

    fprintf(f, "%ld", nfiles);
    putc('\0', f);

    LL(res, list)
        File *fi = (File*) list->data;

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
