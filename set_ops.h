#ifndef SET_OPS
#define SET_OPS
#include <glib.h>

gint hash_size_cmp (gpointer a, gpointer b);
GHashTable *set_intersect_s (GHashTable *a, GHashTable *b);
GHashTable *set_intersect (GList *tables);
GHashTable *set_intersect_p (GHashTable *a, ...);
GHashTable *set_new (GHashFunc hash_func, GEqualFunc equal_func, GDestroyNotify destroy);
void set_add (GHashTable *set, gpointer element);
gboolean set_contains (GHashTable *set, gpointer element);
gboolean set_remove (GHashTable *set, gpointer element);

#endif /* SET_OPS */
