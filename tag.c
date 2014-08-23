#include "abstract_file.h"
#include "tag.h"
#include "types.h"

tagdb_value_t *tag_new_default (Tag *t)
{
    if (t->default_value == NULL)
        return default_value(t->type);
    else
        // TODO: This is only a shallow copy
        // so freeing it may result in a an invalid access error
        return copy_value(t->default_value);
}

Tag *new_tag (char *name, int type, tagdb_value_t *default_value)
{
    Tag *t = g_malloc0(sizeof(Tag));
    abstract_file_init(&t->base, name);
    t->type = type;
    if (default_value)
        t->default_value = default_value;
    return t;
}

void tag_destroy (Tag *t)
{
    abstract_file_destroy(&t->base);
    result_destroy(t->default_value);
    g_free(t);
}
