#ifndef _LOG_H_
#define _LOG_H_
#include <stdio.h>

#include "types.h"

/*  macro to log fields in structs. */
#define log_struct(st, field, format, typecast) \
  log_msg("    " #field " = " #format "\n", typecast st->field)
/* We want to use the file localized log_level whenever we call
 * log_msg.
 */

#define DEBUG 0
#define INFO 1
#define WARN 2
#define ERROR 3

#define log_msg(...) log_msg0(FILE_LOG_LEVEL, __VA_ARGS__)

#define delio(LEVEL,...) log_msg1((LEVEL), __FILE__, __LINE__, __VA_ARGS__)

#define debug(...) delio(DEBUG, __VA_ARGS__)
#define info(...) delio(INFO, __VA_ARGS__)
#define warn(...) delio(WARN, __VA_ARGS__)
#define error(...) delio(ERROR, __VA_ARGS__)

void log_open(const char *name, int log_filter);
void log_open0(FILE *f, int log_filter);
void log_close();
void log_hash (GHashTable *hsh);

void log_msg0(int level, const char *format, ...);
void vlog_msg0 (int log_level, const char *format, va_list ap);
void log_msg1 (int log_level, const char *file, int line_number, const char *format, ...);
int log_error (const char *str);
void log_hash (GHashTable *hsh);
void log_list (GList *l);
void lock_log (void);
void unlock_log (void);

#endif
#ifndef FILE_LOG_LEVEL
#define FILE_LOG_LEVEL 0
#endif
