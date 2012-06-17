#ifndef FILE_DRAWER_H
#define FILE_DRAWER_H

#include <glib.h>
#include "file.h"

typedef struct
{
    /* The actual files keyed by their names */
    GHashTable *table;

    /* The union of tags possessed by files in this drawer */
    GHashTable *tags;
} FileDrawer;

/* Returns the slot as a GList of its contents */
GList *file_drawer_as_list (FileDrawer *s);

FileDrawer *file_drawer_new();
void file_drawer_destroy (FileDrawer *s);
File *file_drawer_lookup (FileDrawer *s, char *file_name);
void file_drawer_remove (FileDrawer *s, File *f);
void file_drawer_insert (FileDrawer *s, File *f);

#endif /* FILE_DRAWER_H */
