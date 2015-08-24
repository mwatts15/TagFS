#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <linux/limits.h>
#include "log.h"
#include "sql.h"

static int _get_version(sqlite3 *db);
static int _set_version(sqlite3 *db);
static int _set_version0(sqlite3 *db, int version);
GTree *get_db_backups (sqlite3 *db);
gboolean increment_database_name (gpointer key, gpointer val, gpointer data);
gboolean unlink_func (gpointer key, gpointer val, gpointer data);
int cp (const char *from, const char *to);

char *upgrade_list [] =
{
    "alter table file_tag rename to file_tag_old;"
    "create table file_tag(file integer, tag integer, value blob,"
    " primary key (file,tag),"
    " foreign key (file) references file(id),"
    " foreign key (tag) references tag(id));"

    "insert into file_tag"
    " select file, tag, default_value from file_tag_old,tag"
    " where tag=id;"

    "insert into file_tag"
    " select file, tag, NULL from file_tag_old,tag"
    " where tag is NULL;"

    "drop table file_tag_old;"
    ,
    "alter table file_tag rename to file_tag_old;"
    "create table file_tag(file integer not null, tag integer not null, value blob,"
    " primary key (file,tag),"
    " foreign key (file) references file(id),"
    " foreign key (tag) references tag(id));"

    "insert into file_tag"
    " select file, tag, value from file_tag_old where tag is not null;"

    "drop table file_tag_old;"
    ,
    "drop table tag_union;"
    ,
    "create table tag_alias(id integer, name varchar(255),"
        " foreign key (id) references tag(id));"
};

char *tables =
    /* a table associating tags to files */
    "create table IF NOT EXISTS file_tag(file integer not null, tag integer not null, value blob,"
            " primary key (file,tag),"
            " foreign key (file) references file(id),"
            " foreign key (tag) references tag(id));"

    /* a table of tag names, ids, and default_values to set for files */
    "create table IF NOT EXISTS tag(id integer primary key, name varchar(255), default_value blob);"

    /* a table of file names, ids */
    "create table IF NOT EXISTS file(id integer primary key, name varchar(255));"

    /* a table associating tags to sub-tags. */
    "create table IF NOT EXISTS subtag(super integer, sub integer unique,"
        " foreign key (super) references tag(id),"
        " foreign key (sub) references tag(id));"

    /* a table for additional names for a tag */
    "create table IF NOT EXISTS tag_alias(id integer, name varchar(255) unique,"
        " foreign key (id) references tag(id));"
;
int _sql_exec(sqlite3 *db, char *cmd, const char *file, int line_number)
{
    char *errmsg = NULL;
    int sqlite_res = sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
    if (sqlite_res != 0)
    {
        log_msg1(ERROR, file, line_number, "sqlite3_exec:%s", errmsg);
        sqlite3_free(errmsg);
    }
    return sqlite_res;
}

int _sql_next_row(sqlite3_stmt *stmt, const char *file, int line_number)
{
    int status = sqlite3_step(stmt);
    if ((status == SQLITE_ROW) || (status == SQLITE_DONE))
    {
        return status;
    }
    else
    {
        log_msg1(ERROR, file, line_number, "We didn't finish the select statemnt. Status code: %d",  status);
        return status;
    }
}

int _sql_step (sqlite3_stmt *stmt, const char *file, int line_number)
{
    int status = sqlite3_step(stmt);
    if (status == SQLITE_DONE || status == SQLITE_ROW)
    {
        return status;
    }
    else
    {
        sqlite3 *db = sqlite3_db_handle(stmt);
        const char *msg = sqlite3_errmsg(db);
        log_msg1(ERROR, file, line_number, "sqlite3_step: We couldn't complete the statement: %s(%d)", msg, status);
        return status;
    }
}

void sql_begin_transaction(sqlite3 *db)
{
    sql_exec(db, "begin transaction");
}

void sql_commit(sqlite3 *db)
{
    sql_exec(db, "commit");
}

/* Returns -1 if there's a failure.
 * Returns 0 if the database was empty (version = 0)
 * Returns 1 if the database successfully updated to the current
 * version and the normal create statements don't need to run.
 */
int try_upgrade_db (sqlite3 *db)
{
    return try_upgrade_db0(db, DB_VERSION);
}

int try_upgrade_db0 (sqlite3 *db, int target_version)
{
    int database_version = _get_version(db);

    if (database_version == 0)
    {
        return 0;
    }

    if (database_version < target_version)
    {
        if (database_backup(db))
        {
            error("Couldn't backup the database before upgrading");
            return -1;
        }

        for (int i = database_version; i < target_version; i++)
        {
            sql_begin_transaction(db);
            int res = sql_exec(db, upgrade_list[i - 1]);
            sql_commit(db);
            if (res != SQLITE_OK)
            {
                error("Error upgrading the database. SQLite error code: %d", res);
                return -1;
            }
        }
        _set_version0(db, target_version);
    }
    else if (database_version == target_version)
    {
        return 1;
    }
    else
    {
        error("Database version (%d) is greater than software version (%d).",
                database_version, target_version);
        return -1;
    }
    return 1;
}

struct
{
    const char *dirname;
    const char *base;
    int backups_renamed;
} FILE_NAME_DATA;

int _name_cmp (const void *a, const void *b, G_GNUC_UNUSED gpointer _UNUSED_)
{
    return -(strcmp((char *)a, (char *)b));
}

int database_backup (sqlite3 *db)
{
    int res = 0;
    const char *db_name = sqlite3_db_filename(db, "main");
    char *db_directory = g_path_get_dirname(db_name);
    debug("directory_name = %s", db_directory);
    char *db_base = g_path_get_basename(db_name);
    int backed_up_count = 0;

    GTree *database_names = get_db_backups(db);
    backed_up_count = g_tree_nnodes(database_names);
    FILE_NAME_DATA.dirname = db_directory;
    FILE_NAME_DATA.base = db_base;
    FILE_NAME_DATA.backups_renamed = 0;

    if (backed_up_count > 0)
    {
        g_tree_foreach(database_names, increment_database_name, NULL);
    }

    if (backed_up_count != FILE_NAME_DATA.backups_renamed)
    {
        res = -1;
    }

    g_tree_destroy(database_names);
    g_free(db_base);
    g_free(db_directory);
    return res;
}

gboolean increment_database_name (gpointer key, G_GNUC_UNUSED gpointer val, G_GNUC_UNUSED gpointer data)
{
    static char src[PATH_MAX];
    static char dest[PATH_MAX];
    const char *base = FILE_NAME_DATA.base;
    char *orig_name = key;
    char *past_the_base = orig_name + strlen(base);
    char *number_part = past_the_base + strlen(BKP_PART);
    int n = 0;
    gboolean on_active_db = FALSE;

    if (*past_the_base != 0)
    {
        n = atoi(number_part) + 1;
    }
    else
    {
        on_active_db = TRUE;
    }

    sprintf(src, "%s/%s", FILE_NAME_DATA.dirname, orig_name);
    sprintf(dest, "%s/%s%s%03d", FILE_NAME_DATA.dirname, base, BKP_PART, n);
    if (n >= 1000)
    {
        warn("Possibly overwriting %s: More than 1000 backups are not supported.", src);
    }
    else
    {
        if (on_active_db)
        {
            if (cp(src, dest))
            {
                error("Couldn't copy %s to %s", src, dest);
                return TRUE;
            }
        }
        else
        {
            if (rename(src, dest))
            {
                error("Couldn't rename %s to %s", src, dest);
                return TRUE;
            }
        }
        FILE_NAME_DATA.backups_renamed++;
    }
    return FALSE;
}

gboolean database_clear_backups (sqlite3 *db)
{
    GTree *names = get_db_backups(db);
    g_tree_foreach(names, unlink_func, db);
    g_tree_destroy(names);
    return TRUE;
}

gboolean unlink_func (gpointer key, G_GNUC_UNUSED gpointer val, gpointer data)
{
    char src[PATH_MAX];
    char *orig_name = key;
    sqlite3 *db = data;

    const char *db_name = sqlite3_db_filename(db, "main");
    const char *db_directory = g_path_get_dirname(db_name);

    sprintf(src, "%s/%s", db_directory, orig_name);
    unlink(src);
    g_free((gpointer)db_directory);
    return FALSE;
}

GTree *get_db_backups (sqlite3 *db)
{
    const char *db_name = sqlite3_db_filename(db, "main");
    const char *db_directory = g_path_get_dirname(db_name);
    const char *db_base = g_path_get_basename(db_name);
    debug("directory_name = %s", db_directory);

    DIR *d = opendir(db_directory);

    struct dirent *de = NULL;
    GTree *database_names = g_tree_new_full(_name_cmp, NULL, g_free, NULL);

    while ((de = readdir(d)) != NULL)
    {
        char *dirent_name = de->d_name;
        debug("dirent_name = %s", dirent_name);
        if (strstr(dirent_name, db_base) == dirent_name)
        {
            g_tree_insert(database_names, g_strdup(dirent_name), NULL);
        }
    }
    closedir(d);
    g_free((gpointer)db_directory);
    g_free((gpointer)db_base);
    return database_names;
}

gboolean database_init(sqlite3 *db)
{
    int status = try_upgrade_db(db);
    int res = TRUE;

    sql_begin_transaction(db);
    if (status == 0)
    {
        status = sql_exec(db, tables);
        if (status != SQLITE_OK)
        {
            error("Couldn't set up the database. SQLite error code: %d", status);
            res = FALSE;
        }
        _set_version(db);
    }
    else if (status == -1)
    {
        error("Couldn't set up the database.");
        res = FALSE;
    }
    sql_commit(db);
    return res;
}

sqlite3* sql_init (const char *db_fname)
{
    sqlite3 *sqlite_db;
    int sqlite_flags = SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX;

    /* 256 MB mmap file */
    if (sqlite3_open_v2(db_fname, &sqlite_db, sqlite_flags, NULL) != SQLITE_OK)
    {
        const char *msg = sqlite3_errmsg(sqlite_db);
        error(msg);
    }
    sqlite3_extended_result_codes(sqlite_db, 1);

    sql_exec(sqlite_db, "PRAGMA mmap_size=268435456");
    /* copied from xmms2 settings */
    sql_exec(sqlite_db, "PRAGMA synchronous = OFF");
    sql_exec(sqlite_db, "PRAGMA auto_vacuum = 1");
    sql_exec(sqlite_db, "PRAGMA cache_size = 8000");
    sql_exec(sqlite_db, "PRAGMA temp_store = MEMORY");
    sql_exec(sqlite_db, "PRAGMA foreign_keys = ON");

    /* One minute */
    sqlite3_busy_timeout (sqlite_db, 60000);
    if (database_init(sqlite_db))
    {
        return sqlite_db;
    }
    else
    {
        error("Database initialiation failed, returning NULL for sqlite3 database.");
        return NULL;
    }
}

static int
_sqlite_version_cb (void *pArg, int argc, char **argv, G_GNUC_UNUSED char **columnName)
{
    int *id = pArg;

    if (argc > 0 && argv[0]) {
        *id = atoi (argv[0]);
    } else {
        *id = 0;
    }

    return 0;
}

static int _get_version(sqlite3 *db)
{
    int res;
    sqlite3_exec(db, "pragma user_version", _sqlite_version_cb, &res, NULL);
    return res;
}

static int _set_version0(sqlite3 *db, int version)
{
    int res;
    static char cmd_string[32];
    sprintf(cmd_string, "pragma user_version = %d", version);
    res = sql_exec(db, cmd_string);
    return res;
}

static int _set_version(sqlite3 *db)
{
    int res;
    res = sql_exec(db, "pragma user_version = " DB_VERSION_S);
    return res;
}

/* Copied from http://stackoverflow.com/a/2180788/638671 . Thanks to user 'caf'. */
int cp (const char *from, const char *to)
{
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
        return -1;

    fd_to = open(to, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd_to < 0)
        goto out_error;

    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR)
            {
                goto out_error;
            }
        } while (nread > 0);
    }

    if (nread == 0)
    {
        if (close(fd_to) < 0)
        {
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);

        /* Success! */
        return 0;
    }

  out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    errno = saved_errno;
    return -1;
}

int _sql_prepare (sqlite3 *db, const char *command, sqlite3_stmt **stmtp, const char *file, int line_number)
{
    int res = sqlite3_prepare_v2(db, command, -1, stmtp, NULL);
    if (res != SQLITE_OK)
    {
        log_msg1(ERROR, file, line_number, "sqlite3_prepare: %d", res);
    }
    return res;
}
