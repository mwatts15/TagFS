#ifndef _LOG_H_
#define _LOG_H_
#include <stdio.h>
#include <glib.h>

/*  macro to log fields in structs. */
#define log_struct(st, field, format, typecast) \
  log_msg("    " #field " = " #format "\n", typecast st->field)
/* We want to use the file localized log_level whenever we call
 * log_msg.
 */

typedef enum {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    LOG_MAX
} log_level_t;

/* Used to log at the current level (always shows up)
 * should be used sparingly. debug,info,warn, or error
 * should be prefered
 */
#define log_msg(...) log_msg0(g_log_filtering_level, __VA_ARGS__)

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
void set_log_filter (int filter_level);
const char *log_level_name(int i);

int g_log_filtering_level;

#endif /* _LOG_H_ */
