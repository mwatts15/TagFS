#include <glib.h>
#include "types.h"
#include "abstract_file.h"
#include "file.h"
#include "util.h"
#include "log.h"
#include "tagdb_util.h"
#include "key.h"

/* Files are equivalent if they are have the same record in memory */
gboolean file_equal (gconstpointer a, gconstpointer b)
{
    return g_direct_equal(a, b);
}

guint file_hash (gconstpointer file)
{
    File *f = (File*) file;
    return (TO_S(f) << 17) ^ (g_str_hash(f->base.name));
}

TagTable *tag_table_new()
{
    return g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) result_destroy);
}

void file_init (File *f, char *name)
{
    abstract_file_init(&f->base, name);
    f->tags = tag_table_new();
    f->refcount = 0;
}

File *new_file (char *name)
{
    File *f = g_malloc(sizeof(File));
    file_init(f,name);
    return f;
}


gboolean file_destroy0 (File *f)
{
    /* Doesn't free the memory */
    if (f->refcount)
        return FALSE;
    abstract_file_destroy(&f->base);
    g_hash_table_destroy(f->tags);
    f->tags = NULL;
    return TRUE;
}

gboolean file_destroy (File *f)
{
    gboolean res = file_destroy0(f);
    if (res)
    {
        g_free(f);
    }
    return res;
}

tagdb_key_t file_extract_key (File *f)
{
    tagdb_key_t key = key_new();
    GList *key_elems = g_hash_table_get_keys(f->tags);
    GList *it = key_elems;

    while (it != NULL)
    {
        key_push_end(key, TO_S(it->data));
        it = it->next;
    }
    g_list_free(key_elems);
    return key;
}

gboolean file_has_tags (File *f, tagdb_key_t tags)
{
    if (key_is_empty(tags) && g_hash_table_size(f->tags) == 0)
        return TRUE;
    KL(tags, i)
    {
        debug("file_has_tags tags[i] = %ld", key_ref(tags,i));
        if (!g_hash_table_lookup(f->tags, TO_SP(key_ref(tags, i))))
            return FALSE;

    } KL_END;
    return TRUE;
}

gboolean file_only_has_tags (File *f, tagdb_key_t tags)
{
    if (key_length(tags) == g_hash_table_size(f->tags))
    {
        return file_has_tags(f, tags);
    }
    return FALSE;
}

void file_remove_tag (File *f, file_id_t tag_id)
{
    if (f)
        g_hash_table_remove(f->tags, TO_SP(tag_id));
}

void file_add_tag (File *f, file_id_t tag_id, tagdb_value_t *v)
{
    if (f)
        g_hash_table_insert(f->tags, TO_SP(tag_id), v);
}

gboolean file_is_untagged (File *f)
{
    if (f)
        return (g_hash_table_size(f->tags) == 0);
    return FALSE;
}
