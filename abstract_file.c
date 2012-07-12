#include <glib.h>

#include "abstract_file.h"

char *file_to_string (gpointer f)
{
    if (f)
    return ((AbstractFile*)f)->name;
    return NULL;
}

void set_name (AbstractFile *f, char *new_name)
{
    g_free(f->name);
    f->name = g_strdup(new_name);
}

int file_id_cmp (AbstractFile *f1, AbstractFile *f2)
{
    if (!f1) return 1;
    if (!f2) return -1;
    return f1->id - f2->id;
}

int file_name_cmp (AbstractFile *f1, AbstractFile *f2)
{
    if (!f1) return 1;
    if (!f2) return -1;
    return g_strcmp0(f1->name, f2->name);
}

int file_str_cmp (AbstractFile *f, char *name)
{
    if (!f) return 1;
    if (!name) return -1;
    return g_strcmp0(f->name, name);
}
