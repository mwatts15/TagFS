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
#include "set_ops.h"

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
gulong *path_extract_key (const char *path)
{
    _log_level = 0;
    /* Get the path components */
    char **comps = split_path(path);
    gulong *buf = g_malloc0_n(g_strv_length(comps) + 3, sizeof(gulong));

    int i = 0;
    while (comps[i])
    {
        log_msg("path component: %s\n", comps[i]);
        Tag *t = lookup_tag(DB, comps[i]);
        if (t == NULL)
        {
            log_msg("path_extract_key t == NULL\n");
            g_strfreev(comps);
            g_free(buf);
    //log_msg("here\n");
            return NULL;
        }
        buf[i] = t->id;
        i++;
    }
    g_strfreev(comps);
    return buf;
}

File *path_to_file (const char *path)
{
    char *base = g_path_get_basename(path);
    char *dir = g_path_get_dirname(path);

    gulong *key = path_extract_key(dir);
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

GList *get_tags_list (TagDB *db, const char *path)
{
    gulong *key = path_extract_key(path);
    if (key == NULL) return NULL;

    GList *tags = NULL;
    int skip = 1;
    KL(key, i)
        FileDrawer *d = file_cabinet_get_drawer(db->files, key[i]);
        log_msg("key %ld\n", key[i]);
        if (d)
        {
            GList *this = file_drawer_get_tags(d);
            this = g_list_sort(this, (GCompareFunc) long_cmp);

            GList *tmp = NULL;
            if (skip)
                tmp = g_list_copy(this);
            else
                tmp = g_list_intersection(tags, this, (GCompareFunc) long_cmp);

            g_list_free(this);
            g_list_free(tags);

            tags = tmp;
        }
        skip = 0;
    KL_END(key, i);

    GList *res = NULL;
    LL(tags, list)
        int skip = 0;
        KL(key, i)
            if (TO_S(list->data) == key[i])
            {
                skip = 1;
            }
        KL_END(key, i);
        if (!skip)
        {
            Tag *t = retrieve_tag(db, TO_S(list->data));
            if (t != NULL)
                res = g_list_prepend(res, t);
        }
    LL_END(list);
    g_list_free(tags);
    return res;
}

/* Gets all of the files with the given tags
   as well as all of the tags below this one
   in the tree */
GList *get_files_list (TagDB *db, const char *path)
{
    char **parts = split_path(path);

    GList *res = NULL;
    int skip = 1;
    int i = 0;
    while (parts[i] != NULL)
    {
        char *part = parts[i];
        GList *files = NULL;
        Tag *t = NULL;
        if (( t = lookup_tag(db, part) ))
            files = file_cabinet_get_drawer_l(db->files, t->id);

        files = g_list_sort(files, (GCompareFunc) file_id_cmp);

        GList *tmp;
        if (skip)
            tmp = g_list_copy(files);
        else
            tmp = g_list_intersection(res, files, (GCompareFunc) file_id_cmp);

        g_list_free(res);
        g_list_free(files);
        res = tmp;
        skip = 0;
        i++;
    }

    return res;
}

