// Since the point of this filesystem is to learn FUSE and its
// datastructures, I want to see *everything* that happens related to
// its data structures.  This file contains macros and functions to
// accomplish this.


#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/time.h>
#include <semaphore.h>

#include "log.h"

#define SHOULD_LOG(_ll) (logging_on && _ll >= g_log_filtering_level)

static FILE *log_file = NULL;
static int logging_on = FALSE;
int g_log_filtering_level = 0;
static sem_t log_lock;
void _lock_log (int operation);

/* Space is left over for additional names to be added at
 * run-time by users. This hasn't yet been implemented
 */
const char _level_names[10][10] = {"DEBUG", " INFO", " WARN", "ERROR"};

void log_open(const char *name, int log_filter)
{
    log_open0(fopen(name, "w"), log_filter);
    sem_init(&log_lock, 0, 1);
}

void log_open0(FILE *f, int log_filter)
{
    if (f == NULL)
    {
        perror("logfile");
        exit(EXIT_FAILURE);
    }
    log_file = f;
    setvbuf(log_file, NULL, _IOLBF, 0); // set to line buffering
    logging_on = 1;
    g_log_filtering_level = log_filter;
    _lock_log(LOCK_EX);
    log_msg("============LOG_START===========\n");
}

void log_close()
{
    log_msg("=============LOG_END============\n");
    _lock_log(LOCK_UN);
    if (logging_on)
    {
        fclose(log_file);
    }
    logging_on = 0;
}


void log_msg0 (int log_level, const char *format, ...)
{
    if (!SHOULD_LOG(log_level))
        return;
    va_list ap;
    va_start(ap, format);
    vlog_msg0(log_level, format, ap);
}

void vlog_msg0 (int log_level, const char *format, va_list ap)
{
    /* this is the only method that does any real
     * writing to the log file
     */
    if (!SHOULD_LOG(log_level))
        return;
    vfprintf(log_file, format, ap);
}

void log_msg1 (int log_level, const char *file, int line_number, const char *format, ...)
{
#ifndef NO_LOGGING
    /* Does some extra things like print the log level
     * and line number
     */
    if (!SHOULD_LOG(log_level))
        return;
    va_list ap;
    va_start(ap, format);
    lock_log();
    struct timeval tv;
    int gtod_ret = gettimeofday(&tv, NULL);
    if (!gtod_ret)
    {
        log_msg0(log_level, "%lu %lu:%s:%s:%d:", tv.tv_sec, tv.tv_usec,
                _level_names[log_level], file, line_number);
    }
    else
    {
        log_msg0(log_level, "%s:%s:%d:", _level_names[log_level], file, line_number);
    }
    vlog_msg0(log_level, format, ap);
    log_msg0(log_level, "\n");
    unlock_log();
#endif
}

void _lock_log (int operation)
{
    if (logging_on)
    {
        int fd = fileno(log_file);
        if (fd > 0)
            flock(fd, operation);
    }
}

void lock_log ()
{
    while ((sem_wait(&log_lock) == -1) && errno == EINTR)
        continue;
}

void unlock_log ()
{
    if (sem_post(&log_lock) == -1)
    {
        return;
    }
}

int log_error (const char *str)
{
    int ret = -errno;
    error("%s: %s\n", str, strerror(errno));
    return ret;
}

void log_pair (gpointer key, gpointer val, G_GNUC_UNUSED gpointer not_used)
{
    log_msg("%p=>%p ",  key, val);
}

void log_hash (GHashTable *hsh)
{
    lock_log();
    log_msg("{");
    if (hsh != NULL)
        g_hash_table_foreach(hsh, log_pair, NULL);
    log_msg("}\n");
    unlock_log();
}

void log_list (GList *l)
{
    log_msg("(");
    if (l != NULL)
    {
        log_msg("%p", l->data);
        l = g_list_next(l);
        while (l != NULL)
        {
            log_msg(" %p", l->data);
            l = g_list_next(l);
        }
    }
    log_msg(")\n");
}

void set_log_filter (int filter_level)
{
    g_log_filtering_level = filter_level;
}

const char *log_level_name(int i)
{
    return _level_names[i];
}

int log_level_int (const char *level_name)
{
    for (int i = 0; i < LOG_MAX; i++)
    {
        if (strcmp(_level_names[i], level_name) == 0)
        {
            return i;
        }
    }
    return -1;
}
