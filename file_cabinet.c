#include <glib.h>
#include "util.h"
#include "tagdb_util.h"
#include "file.h"
#include "file_drawer.h"
#include "file_cabinet.h"

FileCabinet *file_cabinet_new ()
{
    return g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) file_drawer_destroy);
}

FileDrawer *file_cabinet_get_drawer (FileCabinet *fc, gulong slot_id)
{
    return (FileDrawer*) g_hash_table_lookup(fc, TO_SP(slot_id));
}

GList *file_cabinet_get_drawer_l (FileCabinet *fc, gulong slot_id)
{
    return file_drawer_as_list(file_cabinet_get_drawer(fc, slot_id));
}

void file_cabinet_remove_drawer (FileCabinet *fc, gulong slot_id)
{
    g_hash_table_remove(fc, TO_SP(slot_id));
}

int file_cabinet_drawer_size (FileCabinet *fc, gulong key)
{
    return file_drawer_size(file_cabinet_get_drawer(fc, key));
}

void file_cabinet_remove (FileCabinet *fc, gulong key, File *f)
{
    FileDrawer *fs = g_hash_table_lookup(fc, TO_SP(key));
    if (fs)
        file_drawer_remove(fs, f);
}

void file_cabinet_remove_v (FileCabinet *fc, gulong *key, File *f)
{
    KL(key, i)
        file_cabinet_remove(fc, key[i], f);
    KL_END(key, i);
}

void file_cabinet_remove_all (FileCabinet *fc, File *f)
{
    file_extract_key(f, key);
    file_cabinet_remove_v(fc, key, f);
}

void file_cabinet_insert (FileCabinet *fc, gulong key, File *f)
{
    FileDrawer *fs = g_hash_table_lookup(fc, TO_SP(key));
    if (fs)
        file_drawer_insert(fs, f);
}

void file_cabinet_insert_v (FileCabinet *fc, gulong *key, File *f)
{
    KL(key, i)
        file_cabinet_insert(fc, key[i], f);
    KL_END(key, i);
}

void file_cabinet_new_drawer (FileCabinet *fc, gulong slot_id)
{
    g_hash_table_insert(fc, TO_SP(slot_id), file_drawer_new());
}
