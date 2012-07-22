#include "params.h"
#include "tagdb.h"
#include "tagdb_fs.h"

int is_directory (const char *path)
{
    int res = 0;
    char *base = g_path_get_basename(path);
    char *dir = g_path_get_dirname(path);
    /*
       tagdb_key_t tag_key = path_extract_key(path);
       GList *tags = get_tags_list(DB, tag_key);
       */

    Tag *base_tag = lookup_tag(DB, base);
    GList *tags = get_tags_list(DB, dir);

    if (g_list_find(tags, base_tag))
        res = 1;
    g_list_free(tags);

    g_free(base);
    g_free(dir);
    return res;
}

// Also returns the file object if it is a file
File *is_file (const char *path)
{
    return path_to_file(path);
}

int tagdb_getattr (const char *path, struct stat *statbuf)
{
    int retstat = -ENOENT;
    if (is_directory(path))
    {
        statbuf->st_mode = DIR_PERMS;
        retstat = 0;
    }
    else if (is_file(path))
    {
        char *fpath = get_file_copies_path(path);
        retstat = lstat(fpath, statbuf);
        g_free(fpath);
    }
    return retstat;
}

int tagdb_rename (const char *path, const char *newpath)
{
    int retstat = 0;

    char *oldbase = g_path_get_basename(path);
    char *newbase = g_path_get_basename(newpath);

    char *olddir = g_path_get_dirname(path);
    char *newdir = g_path_get_dirname(newpath);
    if (is_directory(path))
    {
        Tag *t = lookup_tag(DB, oldbase);
        if (t)
        {
            if (is_directory(newpath))
            {
                retstat = -1;
                errno = EEXIST;
            }
            else
            {
                set_tag_name(t, newbase, DB);
            }
        }
        else
        {
            retstat = -1;
            errno = ENOENT;
        }
    }
    else
    {
        File *f = path_to_file(path);
        if (f)
        {
            int untagging = 0;
            set_file_name(f, newbase, DB);
            remove_file(DB, f);
            // remove the tags from the file
            // add new tags with old values or NULL
            TagTable *old_tags = f->tags;

            if (g_str_has_suffix(newdir, UNTAG_FH))
            {
                untagging = TRUE;
                char *tmp = newdir;
                newdir = g_path_get_dirname(newdir);
                g_free(tmp);
            }

            tagdb_key_t tags = path_extract_key(newdir);

            KL(tags, i)
            {
                if (untagging)
                {
                    remove_tag_from_file(DB, f, tags[i]);
                }
                else
                {
                    tagdb_value_t *v = g_hash_table_lookup(old_tags, TO_SP(tags[i]));
                    if (!v)
                        add_tag_to_file(DB, f, tags[i], NULL);
                }
            } KL_END;
            insert_file(DB, f);
            g_free(tags);
        }
    }
    g_free(newbase);
    g_free(oldbase);
    g_free(olddir);
    g_free(newdir);
    return retstat;
}

int tagdb_mknod (const char *path, mode_t mode, dev_t dev)
{

    int retstat = 0;

    char *base = g_path_get_basename(path);

    File *f = new_file(base);
    char *fpath = tagfs_realpath(f);
    /*
       if (S_ISREG(mode)) {
       retstat = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
       if (retstat >= 0)
       retstat = close(retstat);
       }
       else */
    if (S_ISFIFO(mode)) {
        retstat = mkfifo(fpath, mode);
    } else {
        retstat = mknod(fpath, mode, dev);
    }

    insert_file(DB, f);
    g_free(base);
    g_free(fpath);
    return retstat;
}

int tagdb_create (const char *path, mode_t mode, struct fuse_file_info *fi)
{
    int retstat = 0;
    char *base = g_path_get_basename(path);
    char *dir = g_path_get_dirname(path);

    File *f = new_file(base);
    tagdb_key_t tags = path_extract_key(dir);
    if (!tags)
    {
        log_msg("invalid path in creat\n");
        errno = ENOENT;
        return -1;
    }

    KL(tags, i)
    {
        add_tag_to_file(DB, f, tags[i], NULL);
    } KL_END;
    insert_file(DB, f);

    char *realpath = tagfs_realpath(f);

    int fd = creat(realpath, mode);
    if (fd)
        fi->fh = fd;
    else
        retstat = -1;

    g_free(base);
    g_free(dir);
    g_free(tags);
    return retstat;
}

int tagdb_mkdir (const char *path, mode_t mode)
{
    char *base = g_path_get_basename(path);
    char *dir = g_path_get_dirname(path);
    tagdb_key_t key = path_extract_key(dir);

    Tag *t = lookup_tag(DB, base);
    if (t == NULL)
    {
        // Make a new tag object
        // give it int type with default value 0
        t = new_tag(base, tagdb_int_t, 0);
        // Insert it into the TagDB TagTree
        insert_tag(DB, t);
    }
    /* Calling mkdir adds a tag to the stage list which causes it to appear
       under the directory in which the call was made. Not every file browser
       behaves the same with regard to this (pcmanfm and thunar will show the
       created directory regardless until you refresh, but midnight commander
       will read the whol directory again and loose it), but this will enforce
       some consistency with regard to how they all behave. */
    stage_add(STAGE, key, t->name, t);

    g_free(key);
    g_free(base);
    g_free(dir);
    return 0;
}

int tagdb_rmdir (const char *path)
{
    /* Every file tagged with base name tag will have all tags in the
       dirname of the path removed from it.

       Essentially, it's what would happen if you called unlink on all of
       the files within seperately

       Calling rmdir on a tag with no files associated with it actually
       deletes the tag.
    */
    int retstat = -1;
    char *dir = g_path_get_dirname(path);
    char *base = g_path_get_basename(path);
    tagdb_key_t key = path_extract_key(path);
    Tag *t = lookup_tag(DB, base);
    if (t)
    {
        remove_tag(DB, t);
        retstat = 0;
    }
    stage_remove(STAGE, key, base);
    g_free(dir);
    g_free(base);
    g_free(key);
    return retstat;
}

int tagdb_unlink (const char *path)
{
    int retstat = 0;
    //char *dir = g_path_get_dirname(path);
    //char *base = g_path_get_basename(path);

    File *f = path_to_file(path);
    if (f)
    {
        char *fpath = tagfs_realpath(f);
        if (fpath)
            retstat = unlink(fpath);
        delete_file(DB, f);
        g_free(fpath);
    }

    return retstat;
}

int tagdb_open (const char *path, struct fuse_file_info *f_info)
{
    int retstat = 0;
    int fd;

    // get the file id from the search path if necessary and get
    // the realpath from the id
    char *fpath = get_file_copies_path(path);
    fd = open(fpath, f_info->flags);
    g_free(fpath);

    f_info->fh = fd;
    log_fi(f_info);
    return retstat;
}

File *path_to_file (const char *path)
{
    char *base = g_path_get_basename(path);
    char *dir = g_path_get_dirname(path);
    char *dirbase = g_path_get_basename(dir);


    tagdb_key_t key = path_extract_key(dir);
    File *f = lookup_file(DB, key, base);
    g_free(key);
    log_msg("f  = %p\n", f);

    g_free(base);
    g_free(dir);
    return f;
}

char *tagfs_realpath (File *f)
{
    return tagfs_realpath_i(f->id);
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


int tagdb_readdir (const char *path, )
{
    GList *f = NULL;
    GList *t = NULL;

    if (g_strcmp0(path, "/") == 0)
    {
        f = tagdb_untagged_items(DB);
        t = g_hash_table_get_values(DB->tags);
    }
    else
    {
        f = get_files_list(DB, path);
        t = get_tags_list(DB, path);
    }

    tagdb_key_t tags = path_extract_key(path);

    GList *s = NULL;
    if (tags)
        s = stage_list_position(STAGE, tags);

    s = g_list_sort(s, (GCompareFunc) file_name_cmp);
    t = g_list_sort(t, (GCompareFunc) file_name_cmp);

    GList *combined_tags = g_list_union(s, t, (GCompareFunc) file_name_cmp);

    LL(f, it)
    {
        if (filler(buffer, file_to_string(it->data), NULL, 0))
        {
            log_msg("    ERROR tagfs_readdir filler:  buffer full");
            return -1;
        }

    } LL_END;

    LL(combined_tags, it)
    {
        if (filler(buffer, tag_to_string(it->data), NULL, 0))
        {
            log_msg("    ERROR tagfs_readdir filler:  buffer full");
            return -1;
        }
    } LL_END;

    g_free(tags);
    g_list_free(f);
    g_list_free(t);
    g_list_free(s);
    g_list_free(combined_tags);
}

struct fuse_operations tagdb_oper = {
    .mknod = tagdb_mknod,
    .mkdir = tagdb_mkdir,
    .rmdir = tagdb_rmdir,
    .readdir = tagdb_readdir,
    .getattr = tagdb_getattr,
    .unlink = tagdb_unlink,
    .rename = tagdb_rename,
    .create = tagdb_create,
    .open = tagdb_open
};
