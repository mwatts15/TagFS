#ifndef PATH_UTIL_H
#define PATH_UTIL_H
#include <glib.h>
char *tagfs_realpath(const char *path);
char *translate_path (const char *path);
char *path_to_qstring (const char *path, gboolean is_file_path);
int path_to_file_id (const char *path);
char *get_id_copies_path (const char *path);
#endif /* PATH_UTIL_H */
