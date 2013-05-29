#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "tagdb.h"
#include "tagdb_priv.h"
#include "file_drawer.h"
#include "file_cabinet.h"
#include "abstract_file.h"
#include "file.h"
#include "tag.h"
#include "scanner.h"
#include "tagdb_util.h"
#include "util.h"
#include "types.h"
#include "log.h"
#include "set_ops.h"

static int _log_level = 0;

GList *tagdb_untagged_items (TagDB *db)
{
    return file_cabinet_get_drawer_l(db->files, UNTAGGED);
}

GList *tagdb_all_files (TagDB *db)
{
    return g_hash_table_get_values(db->files_by_id);
}

void set_file_name (File *f, char *new_name, TagDB *db)
{
    remove_file(db, f);
    set_name((AbstractFile*)f, new_name);
    insert_file(db, f);
}

void set_tag_name (Tag *t, char *new_name, TagDB *db)
{
    g_hash_table_remove(db->tag_codes, t->name);
    set_name((AbstractFile*)t, new_name);
    g_hash_table_insert(db->tag_codes, t->name, TO_SP(t->id));
}

void remove_file (TagDB *db, File *f)
{
    file_cabinet_remove_all(db->files, f);
}

TagBucket *tag_bucket_new ()
{
    return g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
}

void tag_bucket_remove (TagDB *db, Tag *t)
{
    g_hash_table_remove(db->tags, TO_SP(t->id));
}

void tag_bucket_insert (TagDB *db, Tag *t)
{
    g_hash_table_insert(db->tags, TO_SP(t->id), t);
}

gulong tag_bucket_size (TagDB *db)
{
    return (gulong) g_hash_table_size(db->tags);
}

gulong tagdb_ntags (TagDB *db)
{
    return tag_bucket_size(db);
}

void insert_tag (TagDB *db, Tag *t)
{
    if (!t->id)
        t->id = ++db->tag_max_id;
    g_hash_table_insert(db->tag_codes, t->name, TO_SP(t->id));
    tag_bucket_insert(db, t);
    file_cabinet_new_drawer(db->files, t->id);
}

void delete_file_flip (File *f, TagDB *db)
{
    delete_file(db, f);
}

void delete_file (TagDB *db, File *f)
{
    db->nfiles--;
    g_hash_table_remove(db->files_by_id, TO_SP(f->id));
    file_cabinet_remove_all(db->files, f);
    file_destroy(f);
}

void insert_file (TagDB *db, File *f)
{
    tagdb_key_t key = file_extract_key(f);
    if (!f->id)
    {
        db->nfiles++;
        f->id = ++db->file_max_id;
    }

    g_hash_table_insert(db->files_by_id, TO_SP(f->id), f);

    if (file_is_untagged(f))
    {
        file_cabinet_insert(db->files, UNTAGGED, f);
    }
    else
    {
        file_cabinet_insert_v(db->files, key, f);
    }
    g_free(key);
}

File *retrieve_file (TagDB *db, file_id_t id)
{
    return g_hash_table_lookup(db->files_by_id, TO_SP(id));
}

File *lookup_file (TagDB *db, tagdb_key_t keys, char *name)
{
    if (keys == NULL) return NULL;
    File *f = NULL;
    int n = 0;
    KL(keys, i)
    {
        FileDrawer *fs = file_cabinet_get_drawer(db->files, keys[i]);
        f = file_drawer_lookup(fs, name);
        if (f && file_has_tags(f, keys))
        {
            return f;
        }
        n = i;
    } KL_END;
    if (n == 0)
    {
        FileDrawer *fs = file_cabinet_get_drawer(db->files, UNTAGGED);
        f = file_drawer_lookup(fs, name);
        log_msg("\nfile in lookup_file = %s\n", file_to_string(f));
    }
    return f;
}

Tag *retrieve_tag (TagDB *db, file_id_t id)
{
    return (Tag*) g_hash_table_lookup(db->tags, TO_SP(id));
}

file_id_t tag_name_to_id (TagDB *db, char *tag_name)
{

    file_id_t id = TO_S(g_hash_table_lookup(db->tag_codes, tag_name));
    return id;
}

void remove_tag (TagDB *db, Tag *t)
{
    tag_bucket_remove(db, t);
    g_hash_table_remove(db->tag_codes, t->name);
    file_cabinet_remove_drawer(db->files, t->id);
}

Tag *lookup_tag (TagDB *db, char *tag_name)
{
    return retrieve_tag(db, tag_name_to_id(db, tag_name));
}

void remove_tag_from_file (TagDB *db, File *f, file_id_t tag_id)
{
    file_remove_tag(f, tag_id);
    //file_cabinet_remove(db->files, tag_id, f);
}

void add_tag_to_file (TagDB *db, File *f, file_id_t tag_id, tagdb_value_t *v)
{
    /* Look up the tag and return if it can't be found */
    Tag *t = retrieve_tag(db, tag_id);
    if (t == NULL)
    {
        result_destroy(v);
        return;
    }

    /* If it is found, insert the value */
    if (v == NULL)
        v = tag_new_default(t);
    else
        v = copy_value(v);
    file_add_tag(f, tag_id, v);
    //file_cabinet_insert(db->files, tag_id, f);
}

void tagdb_save (TagDB *db, const char *db_fname)
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

    tags_to_file(db, f);
    files_to_file(db, f);
    fclose(f);
}

void tagdb_destroy (TagDB *db)
{
    g_free(db->db_fname);
    /*// Don't need to do this. Taken care of in one fell swoop
      HL(db->files, it, k, v)
      {
      printf("\n%p %p\n", v, k);
      file_drawer_destroy((FileDrawer*) v);
      HL_END;
      }
      */
    g_hash_table_destroy(db->files);
    g_hash_table_destroy(db->files_by_id);
    log_msg("deleted file cabinet\n");
    HL(db->tags, it, k, v)
    {
        tag_destroy((Tag*) v);
    } HL_END;
    printf("deleted tag table\n");
    g_hash_table_destroy(db->tags);
    g_hash_table_destroy(db->tag_codes);
    g_free(db);
}

TagDB *tagdb_new (const char *db_fname)
{
    TagDB *db = g_malloc(sizeof(struct TagDB));
    db->db_fname = g_strdup(db_fname);

    db->files = file_cabinet_new();
    file_cabinet_new_drawer(db->files, UNTAGGED);

    db->tags = tag_bucket_new();
    db->tag_codes = g_hash_table_new(g_str_hash, g_str_equal);
    db->files_by_id = g_hash_table_new(g_direct_hash, g_direct_equal);
    db->file_max_id = 0;
    db->tag_max_id = 0;
    db->nfiles = 0;
    return db;
}

TagDB *tagdb_load (const char *db_fname)
{
    TagDB *db = tagdb_new(db_fname);

    const char *seps[] = {"\0", NULL};
    Scanner *scn = scanner_new_v(seps);
    if (scanner_set_file_stream(scn, db_fname) == -1)
    {
        fprintf(stderr, "Couldn't open db file\n");
        return NULL;
    }

    tags_from_file(db, scn);
    files_from_file(db, scn);
    log_hash(db->files);
    scanner_destroy(scn);
    return db;
}
