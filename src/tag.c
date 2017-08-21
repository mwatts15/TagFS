#include <string.h>
#include <assert.h>
#include "util.h"
#include "log.h"
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

Tag *new_tag (const char *name, int type, const tagdb_value_t *def)
{
    Tag *t = g_malloc0(sizeof(Tag));
    tag_init(t, name, type, def);
    return t;
}

void tag_init (Tag *t, const char *name, int type, const tagdb_value_t *def)
{
    abstract_file_init(&t->base, name);
    abstract_file_set_type(&t->base, abstract_file_tag_type);
    t->type = type;
    t->default_value = copy_value(def?def:default_value(type));
}

void tag_set_name (Tag *t, const char *name)
{
    abstract_file_set_name(t, name);
}

void tag_set_default_explanation (Tag *t, const char *explanation)
{
    g_free(tag_default_explanation(t));
    tag_default_explanation(t) = g_strdup(explanation);
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
    char *start = buffer.content;
    char *end = buffer.content + buffer.size;

    if (start + strlen(tag_name(t)) >= end)
    {
        ((char*)buffer.content)[0] = 0;
    }
    else
    {
        strncpy(start, tag_name(t), end - start);
    }
    return (char*)buffer.content;
}

gboolean tag_destroy (Tag *t)
{
    /*
     * XXX: The lock on `t' is intentionally held to death. All users of the
     *      tag's lock have timeouts and should fail gracefully if they were
     *      waiting
     */
    if (tag_lock(t) != -1)
    {
        tag_destroy0(t); /* XXX: Actual destruction of the tag */
        return TRUE;
    }
    else
    {
        /* The RETURN_FAIL condition is only for failures besides not locking `t' */
        warn("Couldn't lock the tag (%p) for destruction. Returning without action.", t);
        return FALSE;
    }
}

void tag_destroy1 (Tag *t)
{
    abstract_file_destroy(&t->base);
    result_destroy(t->default_value);
    g_slist_free_full(t->aliases, g_free);
}

void tag_destroy0 (Tag *t)
{
    tag_destroy1(t);
    g_free(t);
}
