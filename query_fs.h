#ifndef QUERY_FS_H
#define QUERY_FS_H
#include "params.h"

size_t query_fs_read (const char *path, char *buf, size_t size, off_t offset,
        struct fuse_file_info *f_info);
int query_fs_readddir (const char *path, void *buffer, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *f_info);
int query_fs_getattr (const char *path, struct stat *statbuf);
int query_fs_handles_path (const char *path);

#endif
