#ifndef SET_OPS
#define SET_OPS
#include <glib.h>

typedef gboolean (*set_predicate) (gpointer key, gpointer value, gpointer data);
typedef GHashTable* (*set_operation) (GHashTable *a, GHashTable *b);

gint hash_size_cmp (GHashTable *a, GHashTable *b);
GHashTable *set_intersect_s (GHashTable *a, GHashTable *b);
GHashTable *set_intersect (GList *tables);
GHashTable *set_intersect_p (GHashTable *a, ...);
GHashTable *set_union_s (GHashTable *a, GHashTable *b);
GHashTable *set_union (GList *sets);
GHashTable *set_difference (GList *sets);
GHashTable *set_difference_s (GHashTable *a, GHashTable *b);
GHashTable *set_new (GHashFunc hash_func, GEqualFunc equal_func, GDestroyNotify destroy);
void set_add (GHashTable *set, gpointer element);
gboolean set_contains (GHashTable *set, gpointer element);
gboolean set_remove (GHashTable *set, gpointer element);
GHashTable *set_subset (GHashTable *hash, set_predicate pred, gpointer user_data);
gboolean set_equal_s (GHashTable *a, GHashTable *b);
int set_cmp_s (GHashTable *a, GHashTable *b);

#endif /* SET_OPS */
