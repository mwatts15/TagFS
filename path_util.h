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
char *tagfs_realpath_i (file_id_t id);

/* Only guaranteed for absolute paths */
gboolean path_has_component_with_prefix (const char *path, const char *prefix);
gboolean path_has_component_with_suffix (const char *path, const char *suffix);
gboolean path_has_component (const char *path, const char *component);

typedef gboolean (*path_test) (const char *path, gconstpointer data);

void chug_path(const char *path, char *first, char *rest);
#endif /* PATH_UTIL_H */
