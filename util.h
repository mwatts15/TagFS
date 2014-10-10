#ifndef UTIL_H
#define UTIL_H
#include <glib.h>
#include <stdio.h>
#define TO_P(i) ((gpointer) (glong) (i))
#define TO_SP(i) ((gpointer) (gulong) (i))
#define TO_64P(i) ((gpointer) (gint64) (i))
#define TO_I(p) ((gint)  (glong) (p))
#define TO_S(p) ((gulong) (p))
#define TO_64(p) ((gint64) (p))
// useful macro
#define HL(hash, it, k, v) \
{ \
    if (hash != NULL) {\
    gpointer k, v; \
    GHashTableIter it; \
    g_hash_table_iter_init(&it, hash); \
    while (g_hash_table_iter_next(&it, &k, &v))

#define HL_END } }

#define LL(list,it) \
    for (GList *it = list; it != NULL; it = it->next)
#define LL_END

#define g_hash_loop(hash, it, k, v) \
    g_hash_table_iter_init(&it, hash); \
while (g_hash_table_iter_next(&it, &k, &v))
#define str_equal(a, b) (g_strcmp0(a, b) == 0)
#define size_to_charp(number) to_charp(sizeof(size_t), number)
#define int64_to_charp(number) to_charp(sizeof(gint64), number)

typedef char* (*ToString) (gpointer);
typedef void (*printer) (const char *, ...);

GList *pathToList (const char *path);
GList *g_list_new_charlist (gchar first, ...);
GList *g_list_new (gpointer first, ...);
GList *g_list_flatten (GList *lol);
gboolean str_isalnum (const char *str);
void print_list (GList *lst, ToString s);
void print_string_list (GList *lst);
void print_hash (GHashTable *hsh);
void print_tree (GTree *tree);
int strv_index (const char **vector, const char *str);
int cmp (gconstpointer a, gconstpointer b);
int long_cmp (gpointer a, gpointer b);
extern int high_water_alloc(void **buf, size_t *bufsize, size_t newsize);
char *to_charp (size_t size, size_t n);
size_t charp_to_size (size_t length, char *s);
#endif /* UTIL_H */
