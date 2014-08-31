#include <glib.h>
#include "log.h"
#include "util.h"
#include "file.h"
#include "file_drawer.h"
#include "key.h"

void file_drawer_destroy (FileDrawer *s)
{
    HL(s->table, it, k, v)
    {
        LL(v, lit)
        {
            file_drawer_remove(s, lit->data);
        }
    } HL_END
    g_hash_table_destroy(s->tags);
    g_hash_table_destroy(s->table);
    s->tags = NULL;
    s->table = NULL;
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
    {
        GList *values = g_hash_table_get_values(s->table);
        GList *res = g_list_flatten(values);
        g_list_free(values);
        return res;
    }
    return NULL;
}

File *file_drawer_lookup1 (FileDrawer *s, char *file_name, int index)
{
    return (File*) g_list_nth_data(file_drawer_lookup(s,file_name), index);
}

GList *file_drawer_lookup (FileDrawer *s, char *file_name)
{
    if (s)
    {
        GList *l = (GList*) g_hash_table_lookup(s->table, file_name);
        return l;
    }
    return NULL;
}

void _remove_from_tag_union(FileDrawer *s, File *f);
void _add_to_tag_union(FileDrawer *s, File *f);

void file_drawer_remove (FileDrawer *s, File *f)
{
    if (s && f)
    {
        _remove_from_tag_union(s, f);
        f->refcount--;
        GList *l = g_hash_table_lookup(s->table, file_name(f));
        if (g_list_next(l) == NULL)
        {
            g_list_free(l);
            g_hash_table_remove(s->table, (gpointer) file_name(f));
        }
        else
        {
            l = g_list_remove(l, f);
            g_hash_table_insert(s->table, (gpointer) file_name(f), l);
        }
    }
}

void file_drawer_insert (FileDrawer *s, File *f)
{
    if (s && f)
    {
        /* This is to handle the case of inserting the
         * same file multiple times */
        GList *l =  g_hash_table_lookup(s->table, file_name(f));
        LL(l, it)
        {
            if (file_equal(it->data, f))
            {
                warn("Attempted to insert a file into the same drawer multiple times");
                return;
            }
        } LL_END;

        _add_to_tag_union(s, f);
        f->refcount++;
        l = g_list_prepend(l, f);
        g_hash_table_insert(s->table, (gpointer) file_name(f), l);
    }
}

void _remove_from_tag_union(FileDrawer *s, File *f)
{
    /* update the tag union */
    tagdb_key_t key = file_extract_key(f);
    KL(key, i)
    {
        gpointer tag_id = TO_SP(key_ref(key,i));
        gulong number_of_files_with_this_tag = TO_S(g_hash_table_lookup(s->tags, tag_id));

        if (number_of_files_with_this_tag  == 1)
            g_hash_table_remove(s->tags, tag_id);
        else
            g_hash_table_insert(s->tags, tag_id, TO_SP(number_of_files_with_this_tag - 1));

    } KL_END;
    key_destroy(key);
}

void _add_to_tag_union(FileDrawer *s, File *f)
{
    tagdb_key_t key = file_extract_key(f);
    KL(key, i)
    {
        gpointer tag_id = TO_SP(key_ref(key,i));
        /* this returns NULL==0 if there wasn't an entry, which is fine */
        gulong number_of_files_with_this_tag = TO_S(g_hash_table_lookup(s->tags, tag_id));
        g_hash_table_insert(s->tags, tag_id, TO_SP(number_of_files_with_this_tag + 1));
    } KL_END;
    key_destroy(key);
}

int file_drawer_size (FileDrawer *fd)
{
    return g_hash_table_size(fd->table);
}
