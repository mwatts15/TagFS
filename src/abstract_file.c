#include <glib.h>
#include <errno.h>
#include <assert.h>

#include "log.h"
#include "abstract_file.h"

void abstract_file_init (AbstractFile *f, const char *name)
{
    f->id = 0;
    g_strlcpy(f->name, name, MAX_FILE_NAME_LENGTH);
    sem_init(&f->file_lock, 0, 1);
}

void abstract_file_destroy (AbstractFile *f)
{
    if (sem_destroy(&f->file_lock) != 0)
    {
        error("abstract_file_destroy: sem_destroy error (%d)", errno);
    }
}

char *abstract_file_to_string (AbstractFile *f, char buffer[MAX_FILE_NAME_LENGTH])
{
    if (f)
    {
        if (!lock_timed_out(abstract_file_lock(f)))
        {
            g_snprintf(buffer, MAX_FILE_NAME_LENGTH, "%ld"FIS"%s", f->id, f->name);
            abstract_file_unlock(f);
        }
        else
        {
            warn("Timeout waiting for lock in abstract_file_to_string");
            buffer[0] = 0;
        }
    }
    else
    {
        buffer[0] = 0;
    }
    return buffer;
}

const char *abstract_file_get_name (AbstractFile *f)
{
    return f->name;
}

void _set_name (AbstractFile *f, const char *new_name)
{
    if (!lock_timed_out(abstract_file_lock(f)))
    {
        g_strlcpy(f->name, new_name, MAX_FILE_NAME_LENGTH);
        abstract_file_unlock(f);
    }
}

int file_id_cmp (AbstractFile *f1, AbstractFile *f2)
{
    int res = 0;
    if (f1 != f2) // required since we could try to double lock ourselves :(
    {
        if (!lock_timed_out(abstract_file_lock(f1)))
        {
            if (!lock_timed_out(abstract_file_lock(f2)))
            {
                abstract_file_id_cmp_no_lock(f1, f2, res);
                abstract_file_unlock(f2);
            }
            else
            {
                warn("Lock timed out in file_id_cmp");
            }
            abstract_file_unlock(f1);
        }
        else
        {
            warn("Lock timed out in file_id_cmp");
        }
    }
    return res;
}

int file_id_cmp_no_lock (AbstractFile *f1, AbstractFile *f2)
{
    int res = 0;
    abstract_file_id_cmp_no_lock(f1, f2, res);
    return res;
}

int file_name_cmp (AbstractFile *f1, AbstractFile *f2)
{
    int res = 0;
    if (f1!=f2) // required since we could try to double lock ourselves :(
    {
        if (!lock_timed_out(abstract_file_lock(f1)))
        {
            if (!lock_timed_out(abstract_file_lock(f2)))
            {
                if (!f1) { res = 1; }
                else if (!f2) { res = -1; }
                else {res = g_strcmp0(f1->name, f2->name); }
                abstract_file_unlock(f2);
            }
            else
            {
                warn("Lock timed out in file_name_cmp");
            }
            abstract_file_unlock(f1);
        }
        else
        {
            warn("Lock timed out in file_name_cmp");
        }
    }
    return res;
}

int file_name_id_cmp (AbstractFile *f1, AbstractFile *f2)
{
    int res = 0;
    if (f1!=f2) // required since we could try to double lock ourselves :(
    {
        if (!lock_timed_out(abstract_file_lock(f1)))
        {
            if (!lock_timed_out(abstract_file_lock(f2)))
            {
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
                abstract_file_unlock(f2);
            }
            else
            {
                warn("Lock timed out in file_name_id_cmp");
            }
            abstract_file_unlock(f1);
        }
        else
        {
            warn("Lock timed out in file_name_id_cmp");
        }
    }
    return res;
}

int file_name_str_cmp (AbstractFile *f, char *name)
{
    int res = 0;
    if (!lock_timed_out(abstract_file_lock(f)))
    {
        if (!f) { res = 1; }
        else if (!name) { res = -1; }
        else { res = g_strcmp0(f->name, name); }
        abstract_file_unlock(f);
    }
    else
    {
        warn("Couldn't acquire lock for file_name_str_cmp");
        return 1;
    }
    return res;
}

file_id_t get_file_id (AbstractFile *f)
{
    return f->id;
}

void set_file_id (AbstractFile *f, file_id_t id)
{
    f->id = id;
}

void abstract_file_set_type (AbstractFile *f, abstract_file_type type)
{
    if (!lock_timed_out(abstract_file_lock(f)))
    {
        assert((type == abstract_file_tag_type)
                || (type == abstract_file_file_type)
                || (type == abstract_file_plugin_tag_type));
        f->file_type = type;
        abstract_file_unlock(f);
    }
}
