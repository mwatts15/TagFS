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

static FILE *_log_file = NULL;
static int _logging = FALSE;
static int _log_filter = 6;
int __log_level = 0;

void log_open(const char *name, int log_filter)
{
    // very first thing, open up the logfile and mark that we got in
    // here.  If we can't open the logfile, we're dead.
    _log_file = fopen(name, "w");
    if (_log_file == NULL) {
        perror("logfile");
        exit(EXIT_FAILURE);
    }

    // set logfile to line buffering
    setvbuf(_log_file, NULL, _IOLBF, 0);
    _logging = 1;
    _log_filter = log_filter;
    log_msg("============LOG_START===========\n");
    _log_level = __log_level; // stops gcc complaining
}

void log_close()
{
    log_msg("=============LOG_END============\n");
    if (_logging)
        fclose(_log_file);
    _logging = 0;
}

// this is the only method that
// does any real writing to the log file
void log_msg0 (const char *format, ...)
{

    if (!_logging || __log_level > _log_filter)
        return;
    va_list ap;
    va_start(ap, format);

    vfprintf(_log_file, format, ap);
}

void _lock_log (int operation)
{
    if (_logging)
    {
        int fd = fileno(_log_file);
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

    int tmp = __log_level;
    __log_level = 0;
    log_msg0("    ERROR %s: %s\n", str, strerror(errno));
    __log_level = tmp;

    return ret;
}

void log_pair (gpointer key, gpointer val, gpointer not_used)
{
    log_msg0("%p=>",  key);
    log_msg0("%p ", val);
}

void log_hash (GHashTable *hsh)
{
    lock_log();
    log_msg0("{");
    if (hsh != NULL)
        g_hash_table_foreach(hsh, log_pair, NULL);
    log_msg0("}");
    log_msg0("\n");
    unlock_log();
}

void log_list (GList *l)
{
    log_msg0("(");
    while (l != NULL)
    {
        log_msg0("%p", l->data);
        if (g_list_next(l) != NULL)
        {
            log_msg0(" ");
        }
        l = g_list_next(l);
    }
    log_msg0(")");
    log_msg0("\n");
}
