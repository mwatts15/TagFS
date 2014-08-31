#ifndef KEY_H
#define KEY_H
#include <glib.h>

typedef unsigned long long key_elem_t;
typedef GArray *tagdb_key_t;

/* key for untagged files */
#define UNTAGGED 0ll

#define KL(key, i) \
    for (int i = 0; key_ref(key, i) != 0; i++)

tagdb_key_t key_new (void);
tagdb_key_t make_key (key_elem_t *args, int nkeys);
tagdb_key_t key_copy (tagdb_key_t k);
void key_destroy (tagdb_key_t k);
void key_push_end (tagdb_key_t k, key_elem_t e);
key_elem_t key_ref (tagdb_key_t k, int index);
int key_is_empty (tagdb_key_t k);
void key_sort (tagdb_key_t k, GCompareFunc c);
guint key_length (tagdb_key_t k);
gboolean key_equal(tagdb_key_t k, tagdb_key_t g);
guint key_hash(const tagdb_key_t k);
void key_insert (tagdb_key_t k, key_elem_t e);

void log_key (tagdb_key_t k);

#define KL_END
#endif /* KEY_H */

