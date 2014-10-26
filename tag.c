#include <string.h>
#include <assert.h>
#include "util.h"
#include "abstract_file.h"
#include "tag.h"
#include "types.h"

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
