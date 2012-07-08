#ifndef RESULT_TO_FS_H
#define RESULT_TO_FS_H

#include "types.h"
#include "params.h"

void tagdb_value_fs_readdir (result_t *r, void *buffer, fuse_fill_dir_t filler);
void tagdb_value_fs_stat (result_t *r, struct stat *statbuf);
size_t tagdb_value_fs_read (result_t *r, char *buf, size_t size, off_t offset);

#endif
