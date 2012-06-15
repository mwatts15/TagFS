/* utilities for manipulating paths */
#include <glib.h>
#include <libgen.h>
#include <string.h>
//#include "query.h"
#include "util.h"
#include "types.h"
#include "scanner.h"
#include "tagfs.h"
#include "params.h"
#include "path_util.h"
#include "log.h"

static int _log_level = 0;
// returns the file in our copies directory corresponding to
// the one in path
// should only be called on regular files since
// directories are only virtual
char *tagfs_realpath (File *f)
{
    char *res = g_strdup_printf("%s/%ld", FSDATA->copiesdir, f->id);
    _log_level = 0;
    log_msg("realpath = \"%s\"\n", res);
    return res;
}

/* splits a path into a NULL terminated array of path components */
char **split_path (const char *path)
{
    return g_strsplit(g_path_skip_root(path), "/", -1);
}

/* Translates the path into a NULL-terminated
   vector of Tag IDs, the key format for our
   FileTrie */
gulong *translate_path (const char *path)
{
    _log_level = 0;
    /* Get the path components */
    char **comps = split_path(path);
    gulong *buf = g_malloc0_n(g_strv_length(comps) + 3, sizeof(gulong));

    int i = 0;
    while (comps[i])
    {
        Tag *t = lookup_tag(DB, comps[i]);
        if (t == NULL)
        {
            log_msg("translate_path t == NULL\n");
            g_strfreev(comps);
            g_free(buf);
            return NULL;
        }
        buf[i+1] = t->id;
        i++;
    }
    g_strfreev(comps);
    print_key(buf);
    return buf;
}

File *path_to_file (const char *path)
{
    char *base = g_path_get_basename(path);
    char *dir = g_path_get_dirname(path);

    gulong *key = translate_path(dir);
    // we say key + 1 to avoid checking every file
    File *f = retrieve_file(DB, key, base);
    log_msg("f  = %p\n", f);

    g_free(base);
    g_free(dir);
    g_free(key);
    return f;
}

// turn the path into a file in the copies directory
// path_to_file_search_string + tagdb_query + tagfs_realpath
// NULL for a file that DNE
char *get_file_copies_path (const char *path)
{
    File *f = path_to_file(path);
    if (f)
        return tagfs_realpath(f);
    else
        return NULL;
}
