#include <glib.h>
#include "log.h"
#include "util.h"
#include "tagdb_util.h"
#include "file.h"
#include "file_cabinet.h"

FileCabinet *file_cabinet_new ()
{
    return g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) file_drawer_destroy);
}

void file_cabinet_destroy (FileCabinet *fc)
{
    g_hash_table_destroy(fc);
}

FileDrawer *file_cabinet_get_drawer (FileCabinet *fc, file_id_t slot_id)
{
    return (FileDrawer*) g_hash_table_lookup(fc, TO_SP(slot_id));
}

GList *file_cabinet_get_drawer_l (FileCabinet *fc, file_id_t slot_id)
{
    return file_drawer_as_list(file_cabinet_get_drawer(fc, slot_id));
}

void file_cabinet_remove_drawer (FileCabinet *fc, file_id_t slot_id)
{
    g_hash_table_remove(fc, TO_SP(slot_id));
}

int file_cabinet_drawer_size (FileCabinet *fc, file_id_t key)
{
    return file_drawer_size(file_cabinet_get_drawer(fc, key));
}

void file_cabinet_remove (FileCabinet *fc, file_id_t key, File *f)
{
    FileDrawer *fs = g_hash_table_lookup(fc, TO_SP(key));
    if (fs)
        file_drawer_remove(fs, f);
    else
        error("Attempting to remove a file drawer that doesn't exists");
}

void file_cabinet_remove_v (FileCabinet *fc, tagdb_key_t key, File *f)
{
    KL(key, i)
    {
        file_cabinet_remove(fc, key_ref(key,i), f);
    } KL_END;
}

void file_cabinet_remove_all (FileCabinet *fc, File *f)
{
    tagdb_key_t key = file_extract_key(f);
    file_cabinet_remove_v(fc, key, f);
    key_destroy(key);
}

void file_cabinet_insert (FileCabinet *fc, file_id_t key, File *f)
{
    FileDrawer *fs = g_hash_table_lookup(fc, TO_SP(key));
    if (fs)
        file_drawer_insert(fs, f);
}

void file_cabinet_insert_v (FileCabinet *fc, const tagdb_key_t key, File *f)
{
    KL(key, i)
    {
        if (g_hash_table_lookup(fc, TO_SP(key_ref(key, i))) == NULL)
        {
            return;
        }
    } KL_END
    KL(key, i)
    {
        file_cabinet_insert(fc, key_ref(key,i), f);
    } KL_END
}

void file_cabinet_new_drawer (FileCabinet *fc, file_id_t slot_id)
{
    g_hash_table_insert(fc, TO_SP(slot_id), file_drawer_new(slot_id));
}

gulong file_cabinet_size (FileCabinet *fc)
{
    return g_hash_table_size(fc);
}
