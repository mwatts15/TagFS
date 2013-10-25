#ifndef TAGDB_FS_H
#define TAGDB_FS_H
#include <glib.h>
#include "subfs.h"
#include "tagdb.h"

/* Translates the path into a NULL-terminated
   vector of Tag IDs, the key format for
   FileCabinet access
   The returned array must be freed after use. */
tagdb_key_t path_extract_key (const char *path);
File *path_to_file (const char *path);

/* Shortcut for realpath */
char *get_file_copies_path (const char *path);

extern subfs_component tagdb_fs_subfs;

#endif /* TAGDB_FS_H */

