#ifndef UTIL_H
#define UTIL_H
#include <glib.h>
#include <stdio.h>
#define TO_P(i) ((gpointer) (glong) (i))
#define TO_I(p) ((gint)  (glong) (p))
// useful macro
#define g_hash_loop(hash, it, k, v) \
    g_hash_table_iter_init(&it, hash); \
    while (g_hash_table_iter_next(&it, &k, &v))
#define str_equal(a, b) (g_strcmp0(a, b) == 0)

GList *pathToList (const char *path);
GList *g_list_new_charlist (gchar first, ...);
GList *g_list_new (gpointer first, ...);
gboolean str_isalnum (const char *str);
void print_list (FILE *out, GList *lst);
void print_string_list (GList *lst);
void print_hash (GHashTable *hsh);
void print_tree (GTree *tree);
int strv_index (const char **vector, const char *str);
#endif /* UTIL_H */
