#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "tagdb.h"
#include "tagdb_priv.h"
#include "scanner.h"
#include "util.h"
#include "types.h"
#include "log.h"
#include "set_ops.h"

//log_file

void set_name (AbstractFile *f, char *new_name)
{
    g_free(f->name);
    f->name = g_strdup(new_name);
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

char *file_to_string (gpointer f)
{
    return ((File*)f)->name;
}

void print_key (gulong *k)
{
    log_msg("<<");
    KL(k, i)
        log_msg("%ld ", k[i]);
    KL_END(k, i);
    log_msg(">>\n");
}

int file_id_cmp (File *f1, File *f2)
{
    if (f1 == NULL) return 1;
    if (f2 == NULL) return -1;
    return f1->id - f2->id;
}

GList *get_tags_list (TagDB *db, gulong *key, GList *files_list)
{
    if (key == NULL) return NULL;
    GList *tags = NULL;
    LL(files_list, list)
        if (list->data != NULL)
        {
            File *f = list->data;
            if (f->tags != NULL)
            {
                GList *this = g_hash_table_get_keys(f->tags);
                this = g_list_sort(this, (GCompareFunc) long_cmp);

                GList *tmp = g_list_union(tags, this, (GCompareFunc) long_cmp);

                g_list_free(this);
                g_list_free(tags);

                tags = tmp;
            }
        }
    LL_END(list);

    GList *res = NULL;
    LL(tags, list)
        int skip = 0;
        KL(key, i)
            if (TO_S(list->data) == key[i])
            {
                skip = 1;
            }
        KL_END(key, i);
        if (!skip)
        {
            Tag *t = retrieve_tag(db, TO_S(list->data));
            if (t != NULL)
                res = g_list_prepend(res, t);
        }
    LL_END(list);
    g_list_free(tags);
    return res;
}

/* Gets all of the files with the given tags
   as well as all of the tags below this one
   in the tree */
GList *get_files_list (TagDB *db, gulong *tags)
{
    if (tags == NULL) return NULL;
    GList *res = retrieve_file_slot_l(db, 0);
    res = g_list_sort(res, (GCompareFunc) file_id_cmp);

    KL(tags, i)
        GList *files = retrieve_file_slot_l(db, tags[i]);
        files = g_list_sort(files, (GCompareFunc) file_id_cmp);

        GList *tmp = g_list_intersection(res, files, (GCompareFunc) file_id_cmp);

        g_list_free(res);
        g_list_free(files);
        res = tmp;
        printf("res: ");
        print_list(res, file_to_string);
    KL_END(tags, i);

    return res;
}

void remove_file (TagDB *db, File *f)
{
    file_bucket_remove_all(db, f);
}

TagTable *tag_table_new()
{
    return g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) result_destroy);
}

Tag *new_tag (char *name, int type, gpointer default_value)
{
    Tag *t = g_malloc0(sizeof(Tag));
    t->id = 0;
    t->name = g_strdup(name);
    t->type = type;
    if (default_value != NULL)
        t->default_value = encapsulate(type, default_value);
    return t;
}

File *new_file (char *name)
{
    File *f = g_malloc(sizeof(File));
    f->id = 0;
    f->name = g_strdup(name);
    f->tags = tag_table_new();
    return f;
}

void file_extract_key0 (File *f, gulong *buf)
{
    GList *keys = g_hash_table_get_keys(f->tags);
    GList *it = keys;

    buf[0] = 0;
    int i = 1;
    while (it != NULL)
    {
        buf[i] = TO_S(it->data);
        i++;
        it = it->next;
    }
    g_list_free(keys);
    buf[i] = 0;
    print_key(buf);
}

void file_destroy (File *f)
{
    g_free(f->name);
    g_hash_table_destroy(f->tags);
    f->tags = NULL;
    g_free(f);
}

void tag_destroy (Tag *t)
{
    g_free(t->name);
    result_destroy(t->default_value);
    g_free(t);
}

void file_slot_destroy (FileSlot *s)
{
    g_hash_table_destroy(s);
}

FileBucket *file_bucket_new ()
{
    return g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) file_slot_destroy);
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

FileSlot *file_slot_new()
{
    return (FileSlot*) g_hash_table_new(g_str_hash, g_str_equal);
}

GList *file_slot_as_list (FileSlot *s)
{
    if (s)
        return g_hash_table_get_values(s);
    return NULL;
}

File *file_slot_lookup (FileSlot *s, char *file_name)
{
    if (s)
        return (File*) g_hash_table_lookup(s, file_name);
    return NULL;
}

void file_slot_remove (FileSlot *s, File *f)
{
    if (s)
        g_hash_table_remove(s, f->name);
}

void file_slot_insert (FileSlot *s, File *f)
{
    if (s)
        g_hash_table_insert(s, f->name, f);
}

FileSlot *retrieve_file_slot (TagDB *db, gulong slot_id)
{
    return (FileSlot*) g_hash_table_lookup(db->files, TO_SP(slot_id));
}

GList *retrieve_file_slot_l (TagDB *db, gulong slot_id)
{
    return file_slot_as_list(retrieve_file_slot(db, slot_id));
}

void remove_file_slot (TagDB *db, gulong slot_id)
{
    g_hash_table_remove(db->files, TO_SP(slot_id));
}

void insert_tag (TagDB *db, Tag *t)
{
    if (!t->id)
        t->id = ++db->tag_max_id;
    g_hash_table_insert(db->tag_codes, t->name, TO_SP(t->id));
    tag_bucket_insert(db, t);
    add_new_file_slot(db, t->id);
}

void file_bucket_remove (TagDB *db, gulong key, File *f)
{
    FileSlot *fs = g_hash_table_lookup(db->files, TO_SP(key));
    file_slot_remove(fs, f);
}

void file_bucket_remove_v (TagDB *db, gulong *key, File *f)
{
    KL(key, i)
        file_bucket_remove(db, key[i], f);
    KL_END(key, i);
}

void file_bucket_remove_all (TagDB *db, File *f)
{
    file_extract_key(f, key);
    file_bucket_remove_v(db, key, f);
}

void file_bucket_insert (TagDB *db, gulong key, File *f)
{
    FileSlot *fs = g_hash_table_lookup(db->files, TO_SP(key));
    file_slot_insert(fs, f);
}

void file_bucket_insert_v (TagDB *db, gulong *key, File *f)
{
    KL(key, i)
        file_bucket_insert(db, key[i], f);
    KL_END(key, i);
}

void delete_file_flip (File *f, TagDB *db)
{
    delete_file(db, f);
}

void delete_file (TagDB *db, File *f)
{
    db->nfiles--;
    file_bucket_remove_all(db, f);
    file_destroy(f);
}

void insert_file (TagDB *db, File *f)
{
    file_extract_key(f, key);
    if (!f->id)
    {
        db->nfiles++;
        f->id = ++db->file_max_id;
    }
    file_bucket_insert_v(db, key, f);
}

void add_new_file_slot (TagDB *db, gulong slot_id)
{
    g_hash_table_insert(db->files, TO_SP(slot_id), file_slot_new());
}

int file_str_cmp (File *f, char *name)
{
    return g_strcmp0(f->name, name);
}

File *retrieve_file (TagDB *db, gulong *tag, char *name)
{
    if (tag == NULL) return NULL;
    KL(tag, i)
        FileSlot *fs = retrieve_file_slot(db, tag[i]);
        File *f = file_slot_lookup(fs, name);
        if (f)
            return f;
    KL_END(tag, i);
    return NULL;
}

tagdb_value_t *tag_new_default (Tag *t)
{
    if (t->default_value == NULL)
        return default_value(t->type);
    else
        return copy_value(t->default_value);
}

Tag *retrieve_tag (TagDB *db, gulong id)
{
    return (Tag*) g_hash_table_lookup(db->tags, TO_SP(id));
}

gulong tag_name_to_id (TagDB *db, char *tag_name)
{

    gulong id = TO_S(g_hash_table_lookup(db->tag_codes, tag_name));
    return id;
}

void remove_tag (TagDB *db, Tag *t)
{
    tag_bucket_remove(db, t);
    g_hash_table_remove(db->tag_codes, t->name);
    remove_file_slot(db, t->id);
}

Tag *lookup_tag (TagDB *db, char *tag_name)
{
    return retrieve_tag(db, tag_name_to_id(db, tag_name));
}

void remove_tag_from_file (TagDB *db, File *f, gulong tag_id)
{
    if (f)
        g_hash_table_remove(f->tags, TO_SP(tag_id));
}

void add_tag_to_file (TagDB *db, File *f, gulong tag_id, tagdb_value_t *v)
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
    g_hash_table_insert(f->tags, TO_SP(t->id), v);
    
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
    
    HL(db->files, it, k, v)
        printf("\n%p %p\n", v, k);
        HL((GHashTable*) v, itt, l, w)
            delete_file(db, (File*) w);
        HL_END;
    HL_END;
    
    g_hash_table_destroy(db->files);
    HL(db->tags, it, k, v)
        tag_destroy((Tag*) v);
    HL_END
    g_hash_table_destroy(db->tags);
    g_hash_table_destroy(db->tag_codes);
    g_free(db);
}

TagDB *tagdb_load (const char *db_fname)
{
    TagDB *db = g_malloc(sizeof(struct TagDB));
    char cwd[PATH_MAX];
    getcwd(cwd, PATH_MAX);
    db->db_fname = g_strdup_printf("%s/%s", cwd, db_fname);
    //("db name: %s\n", db->db_fname);
    
    db->files = file_bucket_new();
    add_new_file_slot(db, 0);

    db->tags = tag_bucket_new();
    db->tag_codes = g_hash_table_new(g_str_hash, g_str_equal);
    db->file_max_id = 0;
    db->tag_max_id = 0;
    db->nfiles = 0;
    
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
    //("%ld Files\n", db->nfiles);
    scanner_destroy(scn);
    return db;
}
