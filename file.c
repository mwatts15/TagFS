#include <glib.h>
#include "types.h"
#include "file.h"
#include "util.h"
#include "log.h"
#include "tagdb_util.h"

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

void file_extract_key0 (File *f, gulong *buf)
{
    GList *keys = g_hash_table_get_keys(f->tags);
    GList *it = keys;

    int i = 0;
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

gboolean file_has_tags (File *f, gulong *tags)
{
    if (tags[0] == UNTAGGED && g_hash_table_size(f->tags) == 0) // means we tags is empty i.e. {0, 0, 0}
        return TRUE;
    KL(tags, i)
        log_msg("file_has_tags tags[i] = %ld\n", tags[i]);
        if (!g_hash_table_lookup(f->tags, TO_SP(tags[i])))
            return FALSE;
    KL_END(tags, i);
    return TRUE;
}

void file_remove_tag (File *f, gulong tag_id)
{
    if (f)
        g_hash_table_remove(f->tags, TO_SP(tag_id));
}

void file_add_tag (File *f, gulong tag_id, tagdb_value_t *v)
{
    if (f)
        g_hash_table_insert(f->tags, TO_SP(tag_id), v);
}
