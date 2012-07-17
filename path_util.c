/* utilities for manipulating paths */
#include <glib.h>
#include <libgen.h>
#include <string.h>
#include "query.h"
#include "util.h"
#include "types.h"
#include "scanner.h"
#include "tagfs.h"
#include "params.h"
#include "path_util.h"
#include "log.h"
#include "set_ops.h"
#include "search_to_fs.h"

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

char *tagfs_realpath (File *f)
{
    return tagfs_realpath_i(f->id);
}

/* splits a path into a NULL terminated array of path components */
char **split_path (const char *path)
{
    return g_strsplit(g_path_skip_root(path), "/", -1);
}

/* Translates the path into a NULL-terminated
   vector of Tag IDs, the key format for our
   FileTrie */
tagdb_key_t path_extract_key (const char *path)
{
    _log_level = 0;
    /* Get the path components */
    char **comps = split_path(path);
    tagdb_key_t buf = g_malloc0_n(g_strv_length(comps) + 3, sizeof(file_id_t));

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
    char *dirbase = g_path_get_basename(dir);

    File *f = NULL;
    if (strstr(path, "/" SEARCH_PREFIX))
    {
        f = search_fs_get_file(strstr(path, "/" SEARCH_PREFIX));
    }

    if (!f)
    {
        tagdb_key_t key = path_extract_key(dir);
        f = lookup_file(DB, key, base);
        g_free(key);
    }
    log_msg("f  = %p\n", f);

    g_free(base);
    g_free(dir);
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
    tagdb_key_t key = path_extract_key(path);
    if (key == NULL) return NULL;

    GList *tags = NULL;
    int skip = 1;
    KL(key, i)
    {
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
    } KL_END;

    GList *res = NULL;
    LL(tags, list)
    {
        int skip = 0;
        KL(key, i)
        {
            if (TO_S(list->data) == key[i])
            {
                skip = 1;
            }
        } KL_END;
        if (!skip)
        {
            Tag *t = retrieve_tag(db, TO_S(list->data));
            if (t != NULL)
                res = g_list_prepend(res, t);
        }
    } LL_END;
    g_list_free(tags);
    return res;
}

int direct_compare (gpointer a, gpointer b)
{
    return a - b;
}

/* Gets all of the files with the given tags
   as well as all of the tags below this one
   in the tree */
GList *get_files_list (TagDB *db, const char *path)
{
    char **parts = split_path(path);

    GList *res = NULL;
    int skip = 1;
    int i;
    for (i = 0; parts[i] != NULL; i++)
    {
        char *part = parts[i];
        GList *files = NULL;
        Tag *t = lookup_tag(DB, part);
        if (t)
        {
            files = file_cabinet_get_drawer_l(db->files, t->id);
        }
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
    }
    if (i == 0)
    {
        res = file_cabinet_get_drawer_l(db->files, UNTAGGED);
    }

    return res;
}
