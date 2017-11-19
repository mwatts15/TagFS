#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "tagfs_common.h"
#include "util.h"
#include "params.h"
#include "types.h"
#include "log.h"
#include "stage.h"
#include "tagdb.h"
#include "set_ops.h"
#include "path_util.h"
#include "subfs.h"
#include "tagdb_fs.h"
#include "command_fs.h"
#include "sql.h"
#include "command_default.h"
#include "message_dbus.h"
#include "tag.h"
#include "glib_log.h"

/* configuration variables */
char *c_log_level = NULL;
char *c_db_file_name = NULL;
char *c_log_file_name = NULL;
char *c_data_prefix = NULL;
int c_do_logging = FALSE;
int do_drop_db = FALSE;
int do_version = FALSE;

static GOptionEntry command_line_options[] =
{
  { "logging", 'g', 0,
      G_OPTION_ARG_STRING,
      &c_log_level,
      "Enable logging at the specified level",
      "LEVEL"},
  { "log-file", 'l', 0, G_OPTION_ARG_STRING, &c_log_file_name, "The log file", "FILENAME" },
  { "db-file", 'b', 0, G_OPTION_ARG_STRING, &c_db_file_name, "The database file", "FILENAME" },
  { "data-dir", 0, 0, G_OPTION_ARG_STRING, &c_data_prefix, "Location of the data directory", "DIR" },
  { "drop-db", 0, 0,
      G_OPTION_ARG_NONE,
      &do_drop_db,
      "Delete the entire database. "
          "Retains file content, but will be overwritten upon further use", NULL},
  { "version", 0, 0, G_OPTION_ARG_NONE, &do_version, "Show the current tagfs version", NULL},
  {NULL},
};

/* Eats a relative path and spits out an absolute path */
void absolutize(const char *cwd, char **path)
{
    if (!g_path_is_absolute(*path))
    {
        char *tmp = *path;
        *path = g_build_filename(cwd, *path, NULL);
        g_free(tmp);
    }
}

void _process_options (int *argc_ptr, char ***argv_ptr)
{
    GError *error = NULL;
    GOptionContext *context = NULL;

    context = g_option_context_new("- mount a tagfs file system");
    g_option_context_set_ignore_unknown_options(context, TRUE);
    g_option_context_add_main_entries(context, command_line_options, NULL);

    if (!g_option_context_parse(context, argc_ptr, argv_ptr, &error))
    {
        if (error)
        {
            g_print("option parsing failed: %s\n", error->message);
        }
        else
        {

            g_print("option parsing failed: unknown error\n");
        }
        exit(EXIT_FAILURE);
    }
    g_option_context_free(context);
}

struct tagfs_state *process_options (int argc, char **argv)
{
    struct tagfs_state *tagfs_data = g_malloc0(sizeof(struct tagfs_state));
    char *cwd = g_get_current_dir();
    char *prefix = NULL;

    if (!tagfs_data)
    {
        perror("Cannot alloc tagfs_data");
        abort();
    }

    _process_options(&argc, &argv);
    if (do_version)
    {
        printf(TAGFS_VERSION"\n");
        exit(EXIT_SUCCESS);
    }

    if (c_data_prefix)
    {
        prefix = g_strdup(c_data_prefix);
    }
    else
    {
        prefix = g_build_filename(g_get_user_data_dir(), "tagfs", NULL);
    }

    /* absolutize prefix if necessary */
    absolutize(cwd, &prefix);

    if (mkdir(prefix, (mode_t) 0755) && errno != EEXIST)
    {
        perror("could not make data directory");
    }

    if (c_log_level)
    {
        if (c_log_file_name)
        {
            tagfs_data->log_file = g_strdup(c_log_file_name);
        }
        else
        {
            tagfs_data->log_file = g_build_filename(prefix, "tagfs.log", NULL);
        }

        /* absolutize log_file if necessary */
        absolutize(cwd, &tagfs_data->log_file);

        int level = -1;
        if (g_ascii_isdigit(c_log_level[0]))
        {
            char *end = NULL;
            level = strtol(c_log_level, &end, 10);
        }
        else
        {
            level = log_level_int(c_log_level);
        }

        if (level >= 0)
        {
            log_open(tagfs_data->log_file, level);
        }
        g_set_print_handler(glib_log_handler);
        g_set_printerr_handler(glib_log_handler);
    }
    else
    {
        tagfs_data->log_file = NULL;
    }

    tagfs_data->pid_file = g_build_filename(prefix, "tagfs.pid", NULL);
    FILE *pidfile = fopen(tagfs_data->pid_file, "w");
    if (pidfile == NULL)
    {
        error("Couldn't open pid file %s", tagfs_data->pid_file);
    }
    else
    {
        pid_t pid = getpid();
        if (fprintf(pidfile, "%d", pid) <= 0)
        {
            error("Couldn't write to the pid file");
            fprintf(stderr, "Couldn't write to the pid file");
        }

        if (fclose(pidfile) != 0)
        {
            perror("Problem closing the pidfile");
        }
    }

    char *db_fname = NULL;
    if (c_db_file_name)
    {
        db_fname = g_strdup(c_db_file_name);
        debug("cwd = %s", cwd);
        absolutize(cwd, &db_fname);
    }
    else
    {
        debug("prefix = %s", prefix);
        db_fname = g_build_filename(prefix, "tagfs.db", NULL);
    }
    tagfs_data->copiesdir = g_build_filename(prefix, "copies", NULL);

    debug("cwd = %s", cwd);

    if (mkdir(tagfs_data->copiesdir, (mode_t) 0755))
    {
        if (errno != EEXIST)
        {
            error("could not make copies directory");
            fprintf(stderr, "could not make copies directory\n");
            abort();
        }
        else if (do_drop_db)
        {
            DIR *d = opendir(tagfs_data->copiesdir);
            if (d)
            {
                struct dirent *de = NULL;
                char path[PATH_MAX];
                while ((de = readdir(d)) != NULL)
                {
                    snprintf(path, PATH_MAX, "%s/%s", tagfs_data->copiesdir, de->d_name);
                    if (unlink(path) && !(strcmp(path, ".") == 0 || strcmp(path, "..") == 0))
                    {
                        warn("Couldn't remove a file %s from the copies directory", de->d_name);
                    }
                }
                if (closedir(d) == -1)
                {
                    warn("could not close copies directory handle");
                    fprintf(stderr, "could not close copies directory handle\n");
                }
            }
        }
    }

    debug("argc = %d",argc);
    debug("tagfs_data->copiesdir = \"%s\"", tagfs_data->copiesdir);

    if (argc < 2)
    {
        error("Must provide mount point for %s", argv[0]);
        fprintf(stderr, "Must provide mount point for %s\n", argv[0]);
        abort();
    }

    for (int i = 0; i < argc; i++)
    {
        debug("argv[%d] = '%s'", i, argv[i]);
    }

    if (do_drop_db)
    {
        unlink(db_fname);
    }

    sqlite3* sqldb = sql_init(db_fname);
    if (!sqldb)
    {
        fprintf(stderr, "Couldn't set up the database. Exiting.\n");
        g_free(cwd);
        abort();
    }

    tagfs_data->db = tagdb_new1(sqldb, 0);
    if (!tagfs_data->db)
    {
        fprintf(stderr, "Couldn't set up the database. Exiting.\n");
        g_free(cwd);
        abort();
    }

    debug("setting up the stage");
    tagfs_data->stage = new_stage();
    tagfs_data->command_manager = command_init();
    command_manager_handler_register(tagfs_data->command_manager,
            NULL, command_default_handler);
    char buf[32];
    snprintf(buf, 32, "/tagfs/%d", getpid());
    tagfs_data->mess_conn = dbus_init(buf, "cc.markw.tagfs.fileEvents");
    subfs_init();
    subfs_register_component(&command_fs_subfs);
    subfs_register_component(&tagdb_fs_subfs);
    subfs_init_components();
    tagfs_data->plugin_manager = plugin_manager_new();
    tagfs_data->root_tag = new_tag(".ROOT", 0, NULL);
    debug("entering fuse main");

    g_free(prefix);
    g_free(cwd);
    g_free(db_fname);
    debug("leaving main");
    return tagfs_data;
}
