#ifndef FILE_LOG_H
#define FILE_LOG_H
#include "params.h"
void log_fi (struct fuse_file_info *fi);
void log_stat(struct stat *si);
void log_statvfs(struct statvfs *sv);
void log_utime(struct utimbuf *buf);

#endif
