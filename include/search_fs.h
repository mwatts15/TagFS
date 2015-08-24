#ifndef SEARCH_TO_FS_H
#define SEARCH_TO_FS_H
#include <glib.h>
#include "file.h"

typedef GHashTable SearchList;
typedef GHashTable SearchResult;

void search_fs_insert (const char *search_str);
void search_fs_readdir (const char *search_str, fuse_fill_dir_t filler, void *buffer);
void search_fs_stat (const char *path, struct stat *statbuf);
void search_result_destroy (SearchResult *sr);
File *search_fs_get_file (const char *path);
SearchList *new_search_list ();

#endif
