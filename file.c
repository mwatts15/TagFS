#include <glib.h>
#include "types.h"
#include <assert.h>
#include "abstract_file.h"
#include "file.h"
#include "util.h"
#include "log.h"
#include "tagdb_util.h"
#include "key.h"

gboolean file_equal (gconstpointer a, gconstpointer b)
{
    return g_direct_equal(a, b);
}

guint file_hash (gconstpointer file)
{
    File *f = (File*) file;
    return (TO_S(f) << 17) ^ (g_str_hash(f->name));
}

TagTable *tag_table_new()
{
    return g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) result_destroy);
}

File *new_file (char *name)
{
    File *f = g_malloc(sizeof(File));
    f->id = 0;
    f->name = g_strdup(name);
    f->tags = tag_table_new();
    return f;
}

void file_destroy (File *f)
{
    if (f->refcount)
        return;
    // remove from global files table
    g_free(f->name);
    g_hash_table_destroy(f->tags);
    f->tags = NULL;
    g_free(f);
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
    key_push_end(key, 0);
    key_push_end(key, 0);
    return key;
}

gboolean file_has_tags (File *f, tagdb_key_t tags)
{
    assert(f);
    if (key_is_empty(tags) && g_hash_table_size(f->tags) == 0)
        return TRUE;
    KL(tags, i)
    {
        log_msg("file_has_tags tags[i] = %ld\n", key_ref(tags,i));
        if (!g_hash_table_lookup(f->tags, TO_SP(key_ref(tags, i))))
            return FALSE;

    } KL_END;
    return TRUE;
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
