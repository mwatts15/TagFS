#include <string.h>
#include <assert.h>
#include "util.h"
#include "log.h"
#include "abstract_file.h"
#include "tag.h"
#include "types.h"

/* TagPathInfo is a list for which each entry is populated with the name of
 * a component in the source path and whether or not the component is a root
 * component
 */
struct TPI
{
    /* This is data that must be freed with
     * g_strfreev if it is not NULL
     */
    char **freeme;
    /* This is the actual list of elements
     */
    GList *elements;
};

typedef struct TPIIterator
{
    /* This the current element in an iteration */
    GList *it;
} TPIIterator;

struct TPEI
{
    /* This is data that must be freed with
     * g_strfreev if it is not NULL
     */
    char *name;
    Tag *tag;
};

tagdb_value_t *tag_new_default (Tag *t)
{
    if (t->default_value == NULL)
        return copy_value(default_value(t->type));
    else
        return copy_value(t->default_value);
}

Tag *new_tag (const char *name, int type, tagdb_value_t *def)
{
    Tag *t = g_malloc0(sizeof(Tag));
    abstract_file_init(&t->base, name);
    abstract_file_set_type(&t->base, abstract_file_tag_type);
    t->type = type;
    if (def)
        t->default_value = def;
    else
        t->default_value = copy_value(default_value(type));
    t->children_by_name = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
    return t;
}

void tag_set_subtag0 (Tag *t, Tag *child);
gboolean tag_set_subtag (Tag *t, Tag *child)
{
    assert(t!=child);
    assert(child);
    assert(t);
    gboolean res = TRUE;
    if (tag_lock(t) != -1){
        if (tag_lock(child) != -1){
            tag_set_subtag0(t, child); // XXX: actually set the subtag
            tag_unlock(child);
        }
        else
        {
            warn("tag_set_subtag: Couldn't lock child tag.");
            res = FALSE;
        }
        tag_unlock(t);
    }
    else
    {
        warn("tag_set_subtag: Couldn't lock parent tag.");
        res = FALSE;
    }
    return res;
}

void tag_set_subtag0 (Tag *t, Tag *child)
{
    if (!g_hash_table_lookup_extended(t->children_by_name, tag_name(child), NULL, NULL))
    {
        Tag *current_parent = tag_parent(child);
        if (current_parent)
        {
            g_hash_table_remove(current_parent->children_by_name, tag_name(child));
        }
        tag_parent(child) = t;
        g_hash_table_insert(t->children_by_name, (gpointer) tag_name(child), child);
    }
}

void tag_remove_subtag_s0 (Tag *t, Tag *child);
gboolean tag_remove_subtag_s (Tag *t, const char *child_name)
{
    assert(t);
    Tag *c = NULL;
    gboolean res = TRUE;
    if (!lock_timed_out(tag_lock(t)))
    {
        if (g_hash_table_lookup_extended(t->children_by_name, child_name, NULL, ((gpointer*)&c)))
        {
            if (!lock_timed_out(tag_lock(c)))
            {
                tag_remove_subtag_s0(t, c);
                tag_unlock(c);
            }
            else
            {
                warn("tag_remove_subtag_s: Couldn't lock child tag (%p)", c);
                res = FALSE;
            }
        }
        else
        {
            warn("tag_remove_subtag_s: Attempted to remove a child (%s) tag not possessed by parent tag (%p)", child_name, t);
            res = FALSE;
        }
        tag_unlock(t);
    }
    else
    {
        warn("tag_remove_subtag_s: Couldn't lock parent tag (%p)", t);
        res = FALSE;
    }
    return res;
}

void tag_remove_subtag_s0 (Tag *t, Tag *child)
{
    if (t == tag_parent(child))
    {
        g_hash_table_remove(t->children_by_name, tag_name(child));
        tag_parent(child) = NULL;
    }
    else
    {
        error("tag_remove_subtag_s0: The tag (%p) points to a child (%p) with a different parent", t, child);
    }
}

gboolean tag_remove_subtag (Tag *t, Tag *child)
{
    return tag_remove_subtag_s(t, tag_name(child));
}

TagPathInfo *tag_process_path(const char *path)
{
    TagPathInfo *tpi = g_malloc0(sizeof(TagPathInfo));
    if (g_str_has_prefix(path, TPS) ||
            g_str_has_suffix(path, TPS) ||
            strlen(path) == 0)
    {
        return tpi;
    }

    char **comps = g_strsplit(path, TPS, -1);
    tpi->freeme = comps;
    GList *elts = NULL;
    for (char **s = comps; *s != 0; s++)
    {
        /* just skip/collapse empty path elements. */
        if ((*s)[0] != 0)
        {
            TagPathElementInfo *data = g_malloc0(sizeof(TagPathElementInfo));
            data->name = *s;
            elts = g_list_prepend(elts, data);
        }
    }

    tpi->elements = g_list_reverse(elts);
    return tpi;
}

void tag_destroy_path_element_info(TagPathElementInfo *tpei)
{
    g_free(tpei);
}

const char *tag_path_element_info_name(TagPathElementInfo *tpei)
{
    return tpei->name;
}

Tag *tag_path_element_info_get_tag(TagPathElementInfo *tpei)
{
    return tpei->tag;
}

void tag_path_element_info_set_tag(TagPathElementInfo *tpei, Tag *t)
{
    tpei->tag = t;
}

void tag_path_info_destroy(TagPathInfo *tpi)
{
    g_strfreev(tpi->freeme);
    g_list_free_full(tpi->elements, (GDestroyNotify)tag_destroy_path_element_info);
    g_free(tpi);
}

void tag_path_info_iterator_init(TagPathInfoIterator *it, TagPathInfo *tpi)
{
    TPIIterator *mit = (TPIIterator*) it;
    mit->it = tpi->elements;
}

gboolean tag_path_info_iterator_next(TagPathInfoIterator *it, TagPathElementInfo **store)
{
    TPIIterator *mit = (TPIIterator*)it;
    if (!mit->it)
    {
        return FALSE;
    }
    *store = (TagPathElementInfo*) mit->it->data;
    mit->it = mit->it->next;
    return TRUE;
}

TagPathElementInfo *tag_path_info_first_element(TagPathInfo *tpi)
{
    if (tpi->elements)
    {
        return tpi->elements->data;
    }
    return NULL;
}

int tag_path_info_length(TagPathInfo *tpi)
{
    return g_list_length(tpi->elements);
}

gboolean tag_path_info_is_empty(TagPathInfo *tpi)
{
    return !(tpi->elements);
}

gboolean tag_traverse (Tag *t, TagTraverseFunc f, gpointer data)
{
    int retstat = TRUE;
    tag_lock(t);
    HL(t->children_by_name, it, k, v)
    {
        tag_lock(v);
        int stat = f(v, data);
        tag_unlock(v);
        if (!stat)
            break;

        if (!(retstat = tag_traverse(v, f, data)))
            break;
    } HL_END;
    tag_unlock(t);
    return retstat;
}

gboolean tag_path_info_add_tags (TagPathInfo *tpi, Tag *t, Tag **last)
{
    const char *subtag_name = NULL;
    gboolean skip = TRUE;
    if (last)
    {
        *last = NULL;
    }

    if (tag_path_info_is_empty(tpi))
    {
        t = NULL;
        goto tag_evaluate_path_end;
    }

    TPIL(tpi, it, tei)
    {
        /* These must be retrieved before getting the child if
         * they are to line-up
         */
        subtag_name = tag_path_element_info_name(tei);
        if (!skip)
        {
            t = tag_get_child(t, subtag_name);
        }

        if (!t)
        {
            goto tag_evaluate_path_end;
        }

        if (strcmp(subtag_name, tag_name(t)) != 0)
        {
            t = NULL;
            goto tag_evaluate_path_end;
        }
        tag_path_element_info_set_tag(tei, t);
        if (last)
        {
            *last = t;
        }
        skip = FALSE;
    } TPIL_END;

    tag_evaluate_path_end:
    return !!(t);
}

Tag *tag_evaluate_path0 (Tag *t, TagPathInfo *tpi)
{
    if (!t)
    {
        return NULL;
    }
    Tag *res;
    int is_fully_resolved = tag_path_info_add_tags(tpi, t, &res);

    if (is_fully_resolved)
    {
        return res;
    }
    else
    {
        return 0;
    }
}

char *tag_path_split_right1(char *path)
{
    char *last = tag_path_get_right1(path);
    if (last)
    {
        *(last - TPS_LENGTH) = 0;
    }
    return last;
}

char *tag_path_get_right1 (const char *path)
{
    char *last = g_strrstr(path, TPS);
    if (!last)
    {
        return NULL;
    }

    return last + TPS_LENGTH;
}

Tag *tag_evaluate_path (Tag *t, const char *path)
{
    TagPathInfo *tpi = tag_process_path(path);
    Tag *res = tag_evaluate_path0(t, tpi);
    tag_path_info_destroy(tpi);
    return res;
}

unsigned long tag_number_of_children(Tag *t)
{
    return g_hash_table_size(t->children_by_name);
}

void tag_set_name (Tag *t, const char *name)
{
    Tag *parent = tag_parent(t);
    if (parent)
    {
        g_hash_table_remove(parent->children_by_name, tag_name(t));
        set_name(t, name);
        g_hash_table_insert(parent->children_by_name, (gpointer) tag_name(t), t);
    }
    else
    {
        set_name(t, name);
    }
}

const char *tag_add_alias (Tag *t, const char *alias)
{
    char *alias_copy = g_strdup(alias);
    t->aliases = g_slist_append(t->aliases, alias_copy);
    return alias_copy;
}

void tag_remove_alias (Tag *t, const char *alias)
{
    GSList *to_remove = NULL;
    SLL(t->aliases, it)
    {
        if (strcmp((const char*)it->data, alias) == 0)
        {
            to_remove = it;
        }
    } SLL_END

    if (to_remove)
    {
        g_free(to_remove->data);
        t->aliases = g_slist_delete_link(t->aliases, to_remove);
    }
}

gboolean tag_has_alias(Tag *t, const char *name)
{
    SLL(t->aliases, it)
    {
        if (strcmp((const char*)it->data, name) == 0)
        {
            return TRUE;
        }
    } SLL_END
    return FALSE;
}

char *tag_to_string1 (Tag *t, char *buffer, size_t buffer_size)
{
    buffer_t b = buffer_wrap(buffer_size, buffer);
    return tag_to_string(t, b);
}

char *tag_to_string (Tag *t, buffer_t buffer)
{
    GList *parents = NULL;

    while (t != NULL)
    {
        parents = g_list_prepend(parents, t);
        t = tag_parent(t);
    }

    char *start = buffer.content;
    char *end = buffer.content + buffer.size;
    LL(parents, it)
    {
        if (start + strlen(tag_name(it->data)) >= end)
        {
            ((char*)buffer.content)[0] = 0;
            goto TAG_TO_STRING_END;
        }
        strncpy(start, tag_name(it->data), end - start);
        start += strlen(tag_name(it->data));
        if (it->next)
        {
            const char *tps = TPS;
            if (start + TPS_LENGTH >= end)
            {
                ((char*)buffer.content)[0] = 0;
                goto TAG_TO_STRING_END;
            }

            for (int i = 0; i < TPS_LENGTH; i++)
            {
                start[i] = tps[i];
            }
            start += TPS_LENGTH;
        }
    } LL_END;

    TAG_TO_STRING_END:
    g_list_free(parents);
    return (char*)buffer.content;
}

gboolean tag_destroy (Tag *t)
{
    /*
     * XXX: The lock on `t' is intentionally held to death. All users of the
     *      tag's lock have timeouts and should fail gracefully if they were
     *      waiting
     */
    GList *acquired_locks = NULL; /* These are tags to be unlocked before exit */
    if (tag_lock(t) != -1)
    {
        /* Start acquiring locks */
        Tag *parent = tag_parent(t);
        if (parent)
        {
            if (tag_lock(parent) == -1)
            {
                error("tag_destroy: Couldn't acquire the lock of parent tag (%p). Returning without deletion", parent);
                goto RET_FAIL;
            }
            acquired_locks = g_list_append(acquired_locks, parent);
        }
        HL (t->children_by_name, it, child_name, child)
        {
            if (tag_lock(child) == -1)
            {
                error("tag_destroy: Couldn't acquire the lock of child tag %p. Returning without deletion", child);
                goto RET_FAIL;
            }
            acquired_locks = g_list_prepend(acquired_locks, child);
        } HL_END;
        /* Finish acquiring locks */


        tag_destroy0(t); /* XXX: Actual destruction of the tag and updating of parent and child links */

        /* Release locks */
        LL(acquired_locks, it)
        {
            tag_unlock(it->data);
        }LL_END
        g_list_free(acquired_locks);

        return TRUE;
    }
    else
    {
        /* The RETURN_FAIL condition is only for failures besides not locking `t' */
        warn("Couldn't lock the tag (%p) for destruction. Returning without action.", t);
        return FALSE;
    }


    RET_FAIL:

    tag_unlock(t);
    LL(acquired_locks, it)
    {
        tag_unlock(it->data);
    }LL_END

    g_list_free(acquired_locks);
    return FALSE;
}

void tag_destroy0 (Tag *t)
{
    /* Children are dumped into the next highest tag.
     *
     * If we want to delete the child tags, it is necessary
     * to do so as a separate step prior to deleting this one
     * in a bottom up fashion. There are too-many external
     * links to Tags to allow for any of them to disappear
     * through anything but an external call to tag_destroy
     * that can account for the links.
     */
    Tag *parent = tag_parent(t);
    if (parent)
    {
        Tag *me = g_hash_table_lookup(parent->children_by_name, tag_name(t));

        assert(me == t);

        g_hash_table_remove(parent->children_by_name, tag_name(t));
        tag_parent(t) = NULL;
    }

    HL (t->children_by_name, it, child_name, child)
    {
        if (parent)
        {
            /* We have to do this rather than call tag_set_subtag because otherwise
             * we would have a concurrent modification.
             */
            tag_parent(child) = parent;
            g_hash_table_insert(parent->children_by_name, child_name, child);
        }
        else
        {
            /* Note that we can't use remove_subtag here because it
             * would be modifying the hash that we're currently
             * iterating through
             */
            tag_parent(child) = NULL;
        }
    } HL_END;

    abstract_file_destroy(&t->base);
    result_destroy(t->default_value);
    g_hash_table_destroy(t->children_by_name);
    g_slist_free_full(t->aliases, g_free);
    g_free(t);
}
