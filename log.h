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

#define log_msg(...) log_msg0(FILE_LOG_LEVEL, __VA_ARGS__)

void log_open(const char *name, int log_filter);
void log_close();
void log_hash (GHashTable *hsh);

void log_msg0(int level, const char *format, ...);
int log_error (const char *str);
void log_hash (GHashTable *hsh);
void log_list (GList *l);
void lock_log (void);
void unlock_log (void);

#endif
#ifndef FILE_LOG_LEVEL
#define FILE_LOG_LEVEL 0
#endif
