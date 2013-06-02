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

#include "log.h"

static FILE *log_file = NULL;
static int logging_on = FALSE;
static int log_filtering_level = 0;

void log_open(const char *name, int log_filter)
{
    // very first thing, open up the logfile and mark that we got in
    // here.  If we can't open the logfile, we're dead.
    log_file = fopen(name, "w");
    if (log_file == NULL)
    {
        perror("logfile");
        exit(EXIT_FAILURE);
    }

    // set logfile to line buffering
    setvbuf(log_file, NULL, _IOLBF, 0);
    logging_on = 1;
    log_msg("============LOG_START===========\n");
}

void log_close()
{
    log_msg("=============LOG_END============\n");
    if (logging_on)
    {
        fclose(log_file);
    }
    logging_on = 0;
}

// this is the only method that
// does any real writing to the log file
void log_msg0 (int log_level, const char *format, ...)
{

    if (!logging_on && log_level > log_filtering_level)
        return;
    va_list ap;
    va_start(ap, format);

    vfprintf(log_file, format, ap);
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
    _lock_log(LOCK_EX);
}

void unlock_log ()
{
    _lock_log(LOCK_UN);
}

int log_error (const char *str)
{
    int ret = -errno;
    log_msg0(log_filtering_level+1, "    ERROR %s: %s\n", str, strerror(errno));
    return ret;
}

void log_pair (gpointer key, gpointer val, gpointer not_used)
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
    log_filtering_level = filter_level;
}
