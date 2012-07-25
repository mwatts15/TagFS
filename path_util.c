/* utilities for manipulating paths */
#include <glib.h>
#include <libgen.h>
#include <string.h>
#include "util.h"
#include "params.h"
#include "path_util.h"
#include "log.h"
#include "tagdb.h"

static int _log_level = 0;
// returns the file in our copies directory corresponding to
// the one in path
// should only be called on regular files since
// directories are only virtual
char *tagfs_realpath_i (file_id_t id)
{
    char *res = g_strdup_printf("%s/%ld", FSDATA->copiesdir, id);
    _log_level = 0;
    log_msg("realpath = \"%s\"\n", res);
    return res;
}

/* splits a path into a NULL terminated array of path components */
char **split_path (const char *path)
{
    return g_strsplit(g_path_skip_root(path), "/", -1);
}

gboolean path_has_component_with_test (const char *path, path_test test, const char *test_data)
{
    if (!path || path[0] == '\0')
    {
        return FALSE;
    }
    chug_path(path, first, rest);
    if (test(first, test_data))
    {
        return TRUE;
    }
    return path_has_component_with_test(rest, test, test_data);
}

gboolean path_has_component_with_prefix (const char *path, const char *prefix)
{
    return path_has_component_with_test(path, (path_test) g_str_has_prefix, prefix);
}

gboolean path_has_component_with_suffix (const char *path, const char *suffix)
{
    return path_has_component_with_test(path, (path_test) g_str_has_suffix, suffix);
}

gboolean path_has_component (const char *path, const char *component)
{
    return path_has_component_with_test(path, (path_test) g_str_equal, component);
}

/* returns the path upto a component that contains the given substring */
char *path_before_component (const char *path, const char *substr)
{
    // search the substring
    // if it's found, seek back to the closest path separator and return
    // the part before it.
    char *res = NULL;
    char *match = strstr(path, substr);
    if (match)
    {
        while (match[0] != '/') match--;
        res = g_strndup(path, match - path); // safe since match != NULL
    }
    return res;
}
