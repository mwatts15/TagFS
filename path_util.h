#ifndef PATH_UTIL_H
#define PATH_UTIL_H
#include <glib.h>

/* Translates the path into a NULL-terminated
   vector of Tag IDs, the key format for our
   FileTrie.
   The returned array must be freed after use. */
gulong *translate_path (const char *path);
char **split_path (const char *path);
File *path_to_file (const char *path);
int path_to_file_id (const char *path);

/* Gets the copies path for the File object */
char *tagfs_realpath (File *f);

/* Shortcut for realpath */
char *get_file_copies_path (const char *path);
#endif /* PATH_UTIL_H */
