#include <glib.h>
#include "util.h"
#include "file.h"
#include "file_drawer.h"
#include "key.h"

void file_drawer_destroy (FileDrawer *s)
{
    g_hash_table_destroy(s->tags);
    g_hash_table_destroy(s->table);
    s->tags = NULL; s->table = NULL;
    g_free(s);
}

FileDrawer *file_drawer_new (file_id_t id)
{
    FileDrawer *f = g_malloc0(sizeof(FileDrawer));
    f->table = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
    f->tags = g_hash_table_new(g_direct_hash, g_direct_equal);
    f->id = id;
    return f;
}

GList *file_drawer_get_tags (FileDrawer *s)
{
    return g_hash_table_get_keys(s->tags);
}

GList *file_drawer_as_list (FileDrawer *s)
{
    if (s)
        return g_hash_table_get_values(s->table);
    return NULL;
}

File *file_drawer_lookup (FileDrawer *s, char *file_name)
{
    if (s)
        return (File*) g_hash_table_lookup(s->table, file_name);
    return NULL;
}

void file_drawer_remove (FileDrawer *s, File *f)
{
    if (s && f)
    {
        tagdb_key_t key = file_extract_key(f);
        KL(key, i)
        {
            file_id_t t = TO_S(g_hash_table_lookup(s->tags, TO_SP(key_ref(key,i))));
            if (t == 1)
                g_hash_table_remove(s->tags, TO_SP(key_ref(key,i)));
            else
                g_hash_table_insert(s->tags, TO_SP(key_ref(key,i)), TO_SP(t-1));
        } KL_END;

        f->refcount--;
        g_hash_table_remove(s->table, f->name);
        key_destroy(key);
    }
}

void file_drawer_insert (FileDrawer *s, File *f)
{
    if (s && f)
    {
        tagdb_key_t key = file_extract_key(f);
        /* update the tag union */
        KL(key, i)
        {
            file_id_t t = TO_S(g_hash_table_lookup(s->tags, TO_SP(key_ref(key,i))));
            g_hash_table_insert(s->tags, TO_SP(key_ref(key,i)), TO_SP(t+1));
        } KL_END;

        f->refcount++;
        g_hash_table_insert(s->table, f->name, f);
        key_destroy(key);
    }
}

int file_drawer_size (FileDrawer *fd)
{
    return g_hash_table_size(fd->table);
}
