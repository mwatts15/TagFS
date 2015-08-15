#include "util.h"
#include "stage.h"
#include "key.h"

#define trie_index(__t) ((__t)->index)

/* Modifies the key given */
GNode *_node_lookup(GNode *n, tagdb_key_t position);

/* Staging tags created with mkdir */
Stage *new_stage ()
{
    Stage *res = g_try_malloc0(sizeof(Stage));
    if (!res)
        return NULL;
    res->tree = g_node_new(TO_SP(-1));
    return res;
}

gboolean stage_lookup (Stage *s, tagdb_key_t position, file_id_t id)
{
    tagdb_key_t lookup_key = key_copy(position);
    key_push_end(lookup_key, id);
    GNode *n = _node_lookup(s->tree, lookup_key);
    key_destroy(lookup_key);
    if (n)
    {
        return (id == TO_S(n->data));
    }
    return FALSE;
}

GNode *_node_lookup (GNode *n, tagdb_key_t position)
{
    if (key_is_empty(position))
    {
        return n;
    }

    key_elem_t child_id = key_pop_front(position);
    GNode *child = g_node_find_child(n, G_TRAVERSE_ALL, TO_SP(child_id));

    if (child)
    {
        return _node_lookup(child, position);
    }
    else
    {
        return child;
    }
}

GNode *_node_lookup_create (GNode *n, tagdb_key_t position)
{
    if (key_is_empty(position))
    {
        return n;
    }

    key_elem_t child_id = key_pop_front(position);
    GNode *child = g_node_find_child(n, G_TRAVERSE_ALL, TO_SP(child_id));

    if (child)
    {
        return _node_lookup_create(child, position);
    }
    else
    {
        GNode *new_child = g_node_new(TO_SP(child_id));
        g_node_insert(n, 0, new_child);
        return _node_lookup_create(new_child, position);
    }
}

gboolean stage_add (Stage *s, tagdb_key_t position, file_id_t item)
{
    tagdb_key_t lookup_key = key_copy(position);
    key_push_end(lookup_key, item);
    _node_lookup_create(s->tree, lookup_key);
    key_destroy(lookup_key);
    return TRUE;
}

gboolean stage_remove (Stage *s, tagdb_key_t position, file_id_t id)
{
    tagdb_key_t lookup_key = key_copy(position);
    key_push_end(lookup_key, id);
    GNode *n = _node_lookup(s->tree, lookup_key);
    gboolean res = FALSE;
    if (n)
    {
        g_node_destroy(n);
        res = TRUE;
    }
    key_destroy(lookup_key);
    return res;
}

void stage_remove_all (Stage *s, file_id_t id)
{
    GNode *n = s->tree;
    while (n)
    {
        n = g_node_find(s->tree, G_IN_ORDER, G_TRAVERSE_ALL, TO_SP(id));
        if (n)
        {
            g_node_destroy(n);
        }
    }
}

GList *stage_list_position (Stage *s, tagdb_key_t position)
{
    tagdb_key_t lookup_key = key_copy(position);
    GNode *n = _node_lookup(s->tree, lookup_key);
    GList *res = NULL;
    if (n)
    {
        GNode *it = n->children;
        while (it)
        {
            res = g_list_prepend(res, it->data);
            it = it->next;
        } LL_END;
    }
    key_destroy(lookup_key);
    return res;
}

void stage_destroy (Stage *s)
{
    g_node_destroy(s->tree);
    g_free(s);
}
