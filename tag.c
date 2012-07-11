#include "tag.h"
#include "types.h"

tagdb_value_t *tag_new_default (Tag *t)
{
    if (t->default_value == NULL)
        return default_value(t->type);
    else
        return copy_value(t->default_value);
}

Tag *new_tag (char *name, int type, gpointer default_value)
{
    Tag *t = g_malloc0(sizeof(Tag));
    t->id = 0;
    t->name = g_strdup(name);
    t->type = type;
    if (default_value != NULL)
        t->default_value = encapsulate(type_syms[t->, default_value);
    return t;
}

void tag_destroy (Tag *t)
{
    g_free(t->name);
    result_destroy(t->default_value);
    g_free(t);
}

