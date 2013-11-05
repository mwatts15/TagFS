#include <glib.h>

#include "abstract_file.h"

void abstract_file_init (AbstractFile *f, char *name)
{
    f->id = 0;
    f->name = g_strdup(name);
    sem_init(&f->file_lock, 0, 1);
}

void abstract_file_destroy (AbstractFile *f)
{
    sem_wait(&f->file_lock);
    g_free(f->name);
    sem_destroy(&f->file_lock);
}

char *file_to_string (AbstractFile *f)
{
    sem_wait(&f->file_lock);
    char *res = NULL;
    if (f)
        return ((AbstractFile*)f)->name;
    return res;
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

int file_name_id_cmp (AbstractFile *f1, AbstractFile *f2)
{
    if (!f1) return 1;
    if (!f2) return -1;
    int name_cmp = g_strcmp0(f1->name, f2->name);
    if (name_cmp == 0)
    {
        return f1->id - f2->id;
    }
    return name_cmp;
}

int file_str_cmp (AbstractFile *f, char *name)
{
    if (!f) return 1;
    if (!name) return -1;
    return g_strcmp0(f->name, name);
}

file_id_t get_file_id (AbstractFile *f)
{
    return f->id;
}
