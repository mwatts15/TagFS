#ifndef _LOG_H_
#define _LOG_H_
#include <stdio.h>

#ifdef TAGFS_BUILD
#include "params.h" // brings in fuse.h
#endif

#include "types.h"

//  macro to log fields in structs.
#define log_struct(st, field, format, typecast) \
  log_msg0("    " #field " = " #format "\n", typecast st->field)
#define log_msg(...) \
__log_level = _log_level; log_msg0(__VA_ARGS__)    

void log_open(const char *name, int log_filter);
void log_close();
void log_hash (GHashTable *hsh);

#ifdef TAGFS_BUILD
void log_fi (struct fuse_file_info *fi);
void log_stat(struct stat *si);
void log_statvfs(struct statvfs *sv);
void log_utime(struct utimbuf *buf);
#endif

void log_msg0(const char *format, ...);
int log_error (const char *str);
void log_hash (GHashTable *hsh);
void log_list (GList *l);

extern int __log_level;
static int _log_level;
#endif
