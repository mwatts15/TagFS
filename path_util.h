#ifndef PATH_UTIL_H
#define PATH_UTIL_H
#include <glib.h>
#include "tagdb.h"

/* Translates the path into a NULL-terminated
   vector of Tag IDs, the key format for
   FileCabinet access
   The returned array must be freed after use. */
char **split_path (const char *path);

/* Gets the copies path for the File object */
char *tagfs_realpath (File *f);

/* Only guaranteed for absolute paths */
gboolean path_has_component_with_prefix (const char *path, const char *prefix);
gboolean path_has_component_with_suffix (const char *path, const char *suffix);
gboolean path_has_component (const char *path, const char *component);

typedef gboolean (*path_test) (const char *path, gconstpointer data);

/* Reads off the first element in a path
   and puts the rest of the path in it's
   second arg */
#define chug_path(path, first_path, rest_path) \
char first_path[strlen(path)]; \
char *rest_path; \
{ \
    char *after_root = g_path_skip_root(path); \
    size_t part_length = strcspn(after_root, "/"); \
    g_memmove(first_path, after_root, part_length); \
    first_path[part_length] = 0; \
    rest_path = after_root + part_length; \
} \

#endif /* PATH_UTIL_H */
