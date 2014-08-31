#include "stage.h"

static void sort_key (tagdb_key_t key)
{
    if (!key) return;
    key_sort(key, cmp);
}

/* Staging tags created with mkdir */
Stage *new_stage ()
{
    Stage *res = g_try_malloc0(sizeof(Stage));
    if (!res)
        return NULL;
    res->data = g_hash_table_new_full((GHashFunc) key_hash, (GEqualFunc) key_equal, (GDestroyNotify)key_destroy, NULL);
    return res;
}
void stage_destroy (Stage *s)
{
    HL(s->data, it, k, v)
    {
        g_list_free(v);
    } HL_END
    g_hash_table_destroy(s->data);
    g_free(s);
}

AbstractFile* stage_lookup (Stage *s, tagdb_key_t position, char *name)
{
    sort_key(position);
    GList *l = g_hash_table_lookup(s->data, position);
    LL(l, it)
    {
        AbstractFile* t = it->data;
        if (strcmp(t->name, name) == 0)
        {
            return t;
        }
    } LL_END;
    return NULL;
}

void stage_add (Stage *s, tagdb_key_t position, char *name, AbstractFile* item)
{
    position = key_copy(position);
    sort_key(position);
    GList *l = g_hash_table_lookup(s->data, position);
    l = g_list_prepend(l, item);
    g_hash_table_insert(s->data, position, l);
}

void stage_remove (Stage *s, tagdb_key_t position, char *name)
{
    sort_key(position);
    GList *l = g_hash_table_lookup(s->data, position);
    LL(l,it)
    {
        AbstractFile *t = it->data;
        if (strcmp(abstract_file_get_name(t), name) == 0)
        {
            l = g_list_remove_link(l, it);
            break;
        }
    } LL_END;

    if (l != NULL)
    {
        g_hash_table_insert(s->data, position, l);
    }
    else
    {
        g_hash_table_remove(s->data, position);
    }
}

GList *stage_list_position (Stage *s, tagdb_key_t position)
{
    sort_key(position);
    return g_list_copy(g_hash_table_lookup(s->data, position));
}
