#include <glib.h>

#include "abstract_file.h"

void abstract_file_init (AbstractFile *f, const char *name)
{
    f->id = 0;
    g_strlcpy(f->name, name, MAX_FILE_NAME_LENGTH);
    sem_init(&f->file_lock, 0, 1);
}

void abstract_file_destroy (AbstractFile *f)
{
    sem_wait(&f->file_lock);
    g_free(f->name);
    sem_destroy(&f->file_lock);
}

char *abstract_file_to_string (AbstractFile *f, char buffer[MAX_FILE_NAME_LENGTH])
{
    if (f)
    {
        sem_wait(&f->file_lock);
        g_snprintf(buffer, MAX_FILE_NAME_LENGTH, "%ld:%s", f->id, f->name);
        sem_post(&f->file_lock);
    }
    return buffer;
}

const char *abstract_file_get_name (AbstractFile *f)
{
    return f->name;
}

void _set_name (AbstractFile *f, const char *new_name)
{
    sem_wait(&f->file_lock);
    g_strlcpy(f->name, new_name, MAX_FILE_NAME_LENGTH);
    sem_post(&f->file_lock);
}

int file_id_cmp (AbstractFile *f1, AbstractFile *f2)
{
    int res = 0;
    if (f1!=f2) // required since we could try to double lock ourselves :(
    {
        sem_wait(&f1->file_lock);
        sem_wait(&f2->file_lock);
        if (!f1) { res = 1; }
        else if (!f2) { res = -1; }
        else {res = f1->id - f2->id; }
        sem_post(&f2->file_lock);
        sem_post(&f1->file_lock);
    }
    return res;
}

int file_name_cmp (AbstractFile *f1, AbstractFile *f2)
{
    int res = 0;
    if (f1!=f2) // required since we could try to double lock ourselves :(
    {
        sem_wait(&f1->file_lock);
        sem_wait(&f2->file_lock);
        if (!f1) { res = 1; }
        else if (!f2) { res = -1; }
        else {res = g_strcmp0(f1->name, f2->name); }
        sem_post(&f2->file_lock);
        sem_post(&f1->file_lock);
    }
    return res;
}

int file_name_id_cmp (AbstractFile *f1, AbstractFile *f2)
{
    int res = 0;
    if (f1!=f2) // required since we could try to double lock ourselves :(
    {
        sem_wait(&f1->file_lock);
        sem_wait(&f2->file_lock);
        if (!f1){  res = 1; }
        else if (!f2) { res = -1; }
        else {
            int name_cmp = g_strcmp0(f1->name, f2->name);
            if (name_cmp == 0)
            {
                res = f1->id - f2->id;
            }
            else
            {
                res = name_cmp;
            }
        }
        sem_post(&f2->file_lock);
        sem_post(&f1->file_lock);
    }
    return res;
}

int file_name_str_cmp (AbstractFile *f, char *name)
{
    int res = 0;
    sem_wait(&f->file_lock);
    if (!f) { res = 1; }
    else if (!name) { res = -1; }
    else { res = g_strcmp0(f->name, name); }
    sem_post(&f->file_lock);
    return res;
}

file_id_t get_file_id (AbstractFile *f)
{
    return f->id;
}
