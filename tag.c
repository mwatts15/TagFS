#include <string.h>
#include <assert.h>
#include "util.h"
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
    t->type = type;
    if (def)
        t->default_value = def;
    else
        t->default_value = copy_value(default_value(type));
    t->children_by_name = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
    return t;
}

void tag_set_subtag (Tag *t, Tag *child)
{
    assert(child);
    assert(t);
    if (g_hash_table_lookup_extended(t->children_by_name, tag_name(child), NULL, NULL))
    {
        return;
    }

    Tag *current_parent = tag_parent(child);
    if (current_parent)
    {
        g_hash_table_remove(current_parent->children_by_name, tag_name(child));
    }
    tag_parent(child) = t;
    g_hash_table_insert(t->children_by_name, (gpointer) tag_name(child), child);
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

gboolean tag_path_info_is_empty(TagPathInfo *tpi)
{
    return !(tpi->elements);
}

void tag_traverse (Tag *t, TagTraverseFunc f, gpointer data)
{
    HL(t->children_by_name, it, k, v)
    {
        f(v, data);
        tag_traverse(v, f, data);
    } HL_END;
}

gboolean tag_path_info_add_tags (TagPathInfo *tpi, Tag *t, Tag **last)
{
    const char *subtag_name = NULL;
    gboolean skip = TRUE;
    *last = NULL;

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
        *last = t;
        skip = FALSE;
    } TPIL_END;

    tag_evaluate_path_end:
    return !!(t);
}

Tag *tag_evaluate_path0 (Tag *t, TagPathInfo *tpi)
{
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

char *tag_to_string (Tag *t, buffer_t buffer)
{
    /* TODO: Ensure this doesn't write past the end of the buffer */
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
            buffer.content = NULL;
            goto TAG_TO_STRING_END;
        }
        strncpy(start, tag_name(it->data), end - start);
        start += strlen(tag_name(it->data));
        if (it->next)
        {
            if (start + 2 >= end)
            {
                buffer.content = NULL;
                goto TAG_TO_STRING_END;
            }
            start[0] = ':';
            start[1] = ':';
            start += 2;
        }
    } LL_END;

    TAG_TO_STRING_END:
    g_list_free(parents);
    return (char*)buffer.content;
}

void tag_destroy (Tag *t)
{
    Tag *parent = tag_parent(t);
    if (parent)
    {
        g_hash_table_remove(parent->children_by_name, tag_name(t));
    }
    /* Children are dumped into the next highest tag.
     *
     * If we want to delete the child tags, it is necessary
     * to do so as a separate step prior to deleting this one
     * in a bottom up fashion. There are too-many external
     * links to Tags to allow for any of them to disappear
     * through anything but an external call to tag_destroy
     * that can account for the links.
     */
    HL (t->children_by_name, it, k, v)
    {
        Tag *child = v;
        if (parent)
        {
            /* We have to do this rather than call tag_set_subtag because otherwise
             * we would have a concurrent modification.
             */
            tag_parent(child) = parent;
            g_hash_table_insert(parent->children_by_name, (gpointer) tag_name(child), child);
        }
        else
        {
            tag_parent(child) = NULL;
        }
    } HL_END;

    abstract_file_destroy(&t->base);
    result_destroy(t->default_value);
    g_hash_table_destroy(t->children_by_name);
    g_free(t);
}
