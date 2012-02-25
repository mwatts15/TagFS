#ifndef UTIL_H
#define UTIL_H
#include <glib.h>
#include <stdio.h>
GList *pathToList (const char *path);
gboolean str_isalnum (const char *str);
void print_list (FILE *out, GList *lst);
void print_hash (GHashTable *hsh);
void print_tree (GTree *tree);
#endif /* UTIL_H */
