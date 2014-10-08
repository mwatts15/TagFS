/* utilities for manipulating paths */
#include <glib.h>
#include <libgen.h>
#include <string.h>
#include "util.h"
#include "path_util.h"
#include "log.h"
#include "tagdb.h"

/* first and rest must be able to hold the entire
 * length of the path
 */
void chug_path(const char *path, char *first, char *rest)
{
    /* part of the path that follows the root path character(s) */
    const char *after_root;
    size_t after_root_length;
    /* length of the first path component */
    size_t part_length;

    after_root = g_path_skip_root(path);
    after_root_length = strlen(after_root);
    part_length = strcspn(after_root, "/");

    g_memmove(first, after_root, part_length);
    first[part_length] = 0;
    g_memmove(rest, after_root + part_length, part_length - after_root_length);
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
    size_t path_length = strlen(path);
    char *first = g_alloca(path_length);
    char *rest = g_alloca(path_length);
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
