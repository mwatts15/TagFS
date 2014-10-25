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
    return t;
}

void tag_destroy (Tag *t)
{
    abstract_file_destroy(&t->base);
    result_destroy(t->default_value);
    g_free(t);
}
