#ifndef _LOG_H_
#define _LOG_H_
#include <stdio.h>
#include <fuse.h>
#include "types.h"

//  macro to log fields in structs.
#define log_struct(st, field, format, typecast) \
  log_msg("    " #field " = " #format "\n", typecast st->field)
#define log_msg(...) \
__log_level = _log_level; log_msg0(__VA_ARGS__)    

void log_open(const char *name, int log_filter);
void log_close();
void log_hash (GHashTable *hsh);
void log_fi (struct fuse_file_info *fi);
void log_stat(struct stat *si);
void log_statvfs(struct statvfs *sv);
void log_utime(struct utimbuf *buf);

void log_msg0(const char *format, ...);
int log_error (char *str);
void log_hash (GHashTable *hsh);
void log_query_info (query_t *q);

extern int __log_level;
static int _log_level;
#endif
