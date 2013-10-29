#ifndef ABSTRACT_FILE_H
#define ABSTRACT_FILE_H
#include <glib.h>

typedef unsigned long file_id_t;

typedef struct AbstractFile
{
    file_id_t id;
    char *name;
} AbstractFile;

char *file_to_string (gpointer f, char* buf);
void set_name (AbstractFile *f, char *new_name);
int file_id_cmp (AbstractFile *f1, AbstractFile *f2);
int file_name_cmp (AbstractFile *f1, AbstractFile *f2);
int file_str_cmp (AbstractFile *f, char *name);

#endif /* ABSTRACT_FILE_H */
