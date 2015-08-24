#ifndef TAGDB_UTIL_H
#define TAGDB_UTIL_H
#include "abstract_file.h"
#include "key.h"
#include "tagdb.h"

GList *get_files_list (TagDB *db, tagdb_key_t key);
GList *get_tags_list (TagDB *db, tagdb_key_t key);

#endif /* TAGDB_UTIL_H */
