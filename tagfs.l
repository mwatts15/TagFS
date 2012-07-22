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
#include <sys/types.h>

#include "util.h"
#include "params.h"
#include "types.h"
#include "log.h"
#include "result_queue.h"
#include "result_fs.h"
#include "search_fs.h"
#include "stage.h"
#include "tagdb.h"
#include "set_ops.h"
#include "query.h"
#include "path_util.h"

static char *consistency_flag_file;

gint64 path_to_queue_key (const char *path)
{
    char *base = g_path_get_basename(path);
    gint64 uid;
    if (g_str_has_prefix(base, QREAD_PREFIX))
    {
        uid = strtoll(base + strlen(QREAD_PREFIX), NULL, 10);
    }
    else if (strstr(path, "/" LISTEN_FH))
    {
        uid = fuse_get_context()->uid;
    }
    else
    {
        uid = 0;
    }
    g_free(base);
    return uid;
}

int is_directory (const char *path)
{
    log_msg("is it a dir?\n");

    if (g_str_equal(path, "/")
            || g_str_has_suffix(path, UNTAG_FH))
        return TRUE;

    int res = 0;
    char *base = g_path_get_basename(path);

    char *dir = g_path_get_dirname(path);
    tagdb_key_t dirtags = path_extract_key(dir);

    Tag *base_tag = lookup_tag(DB, base);
    _log_level = 0;

    if ((g_strcmp0(dir, "/") == 0 && base_tag)
            || stage_lookup(STAGE, dirtags, base))
    {
        res = 1;
    }
    else
    {
        GList *tags = get_tags_list(DB, dir);
        if (g_list_find(tags, base_tag))
            res = 1;
        g_list_free(tags);
    }

    g_free(base);
    g_free(dirtags);
    g_free(dir);
    return res;
}

// Also returns the file object if it is a file
File *is_file (const char *path)
{
    log_msg("is it a file?\n");
    return path_to_file(path);
}

int tagfs_getattr (const char *path, struct stat *statbuf)
{
#if 0
    int retstat = -ENOENT;
    if (g_str_equal(path, "/"))
    {
        statbuf->st_mode = DIR_PERMS;
        retstat = 0;
    }
    else
    {
            gint64 uid = path_to_queue_key(path);
            if (uid == fuse_get_context()->uid)
            {
                statbuf->st_mode = S_IFREG | 0444;
                result_t *res = result_queue_peek(FSDATA->rqm, uid);
                if (res != NULL)
                {
                    binstring_t *bs = tagdb_value_to_binstring(res);
                    statbuf->st_size = bs->size; /* get the size of the "file" */
                    statbuf->st_blksize = 4096; /* based on other files values */
                    binstring_destroy(bs);
                    retstat = 0;
                }
                else
                {
                    retstat = -1;
                }
            }
        g_free(base);
    }
    log_stat(statbuf);
    return retstat;
#endif
}

int tagfs_readlink (const char *path, char *realpath, size_t bufsize)
{
    char *copies_path = get_file_copies_path(path);
    int retstat = readlink(copies_path, realpath, bufsize);
    g_free(copies_path);
    return retstat;
}

int tagfs_rename (const char *path, const char *newpath)
{
    int retstat = 0;
    // do the thing
    return retstat;
}

int tagfs_create (const char *path, mode_t mode, struct fuse_file_info *fi)
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

int tagfs_open (const char *path, struct fuse_file_info *f_info)
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

int tagfs_write (const char *path, __DNP__ const char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi)
{
    _log_level = 0;
    //log_msg("uid = %d\n", fuse_get_context()->uid);

    // file search etc, etc

    // check if we're writing to the "listen" file
    if (g_str_has_suffix(path, LISTEN_FH))
    {
        // sanitize the buffer string
        char *cmdstr = g_strstrip((char*)g_memdup(buf, size + 1));
        cmdstr[size] = '\0';
        result_t *res = query_and_queue_result(cmdstr);
        res_info(res, log_msg0);
        g_free(cmdstr);
        return size;
    }
    return pwrite(fi->fh, buf, size, offset);
}

int tagfs_read (const char *path, __DNP__ char *buffer, size_t size, off_t offset,
        struct fuse_file_info *f_info)
{
    int retstat = 0;
    gint64 uid = path_to_queue_key(path);
    if (uid == fuse_get_context()->uid)
    {
        result_t *res = path_to_result(path);
        if (!res)
        {
            retstat = -1;
        }
        else
        {
            retstat = tagdb_value_fs_read(res, buffer, size, offset);
            res_info(res, log_msg0);
        }
    }
    else
    {
        // no need to get fpath on this one, since I work from f_info->fh not the path
        /* This also works for the file search since we've opened the file */
        log_fi(f_info);

        retstat = pread(f_info->fh, buffer, size, offset);
    }
    return retstat;
}

int tagfs_flush(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;

    return retstat;
}

int tagfs_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
    int retstat = 0;

    log_fi(fi);

    if (datasync)
        retstat = fdatasync(fi->fh);
    else
        retstat = fsync(fi->fh);

    if (retstat < 0)
        log_error("tagfs_fsync fsync");

    return retstat;
}

int tagfs_truncate(const char *path, off_t newsize)
{
    int retstat = 0;

    char *fpath = get_file_copies_path(path);

    if (fpath != NULL)
    {
        retstat = truncate(fpath, newsize);
        if (retstat < 0)
            log_error("tagfs_truncate truncate");
    }
    g_free(fpath);
    return retstat;
}

int tagfs_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;

    retstat = ftruncate(fi->fh, offset);

    return retstat;
}

int tagfs_releasedir (const char *path, struct fuse_file_info *f_info)
{
    /* stub */
    return 0;
}

int tagfs_opendir (const char *path, struct fuse_file_info *f_info)
{
    /* stub */
    return 0;
}

int tagfs_mknod (const char *path, mode_t mode, dev_t dev)
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

int tagfs_mkdir (const char *path, mode_t mode)
{
    // do the thing
    return 0;
}

int tagfs_unlink (const char *path)
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

int tagfs_release (const char *path, struct fuse_file_info *f_info)
{
    log_fi(f_info);
    if (g_str_has_suffix(path, LISTEN_FH))
        return 0;

    return close(f_info->fh);
}

int tagfs_readdir (const char *path, void *buffer, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *f_info)
{
    int retstat = 0;

    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);

    subfs_do_oper(path, readdir, path, buffer, filler, offset, f_info);
#if 0
    if (r)
    {
        tagdb_value_fs_readdir(r, buffer, filler);
        goto EXIT;
    }

    char *search_path = strstr(path, "/" SEARCH_PREFIX);
    if (search_path)
    {
        search_fs_readdir(search_path, filler, buffer);
        goto EXIT;
    }
#endif
EXIT:
    log_msg("leaving readdir\n");
    return retstat;
}

int tagfs_rmdir (const char *path)
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

int tagfs_utime(const char *path, struct utimbuf *ubuf)
{
    int retstat = 0;

    char *fpath = get_file_copies_path(path);

    if (fpath)
    {
        retstat = utime(fpath, ubuf);
        if (retstat < 0)
            retstat = log_error("tagfs_utime utime");
    }
    else
    {
        log_error("tagfs_utime get_file_copies_path");
        retstat = -1;
    }

    g_free(fpath);
    return retstat;
}

int tagfs_access(const char *path, int mask)
{
    int retstat = 0;
    if (is_directory(path)) return 0;

    char *fpath = get_file_copies_path(path);

    retstat = access(fpath, mask);

    if (retstat < 0)
        retstat = log_error("tagfs_access access");

    g_free(fpath);
    return retstat;
}

/** Change the permission bits of a file */
int tagfs_chmod(const char *path, mode_t mode)
{
    int retstat = 0;

    char *fpath = get_file_copies_path(path);

    retstat = chmod(fpath, mode);
    if (retstat < 0)
        retstat = log_error("tagfs_chmod chmod");

    g_free(fpath);
    return retstat;
}

/** Change the owner and group of a file */
int tagfs_chown(const char *path, uid_t uid, gid_t gid)
{
    int retstat = 0;

    char *fpath = get_file_copies_path(path);

    retstat = chown(fpath, uid, gid);
    if (retstat < 0)
        retstat = log_error("tagfs_chown chown");

    g_free(fpath);
    return retstat;
}

void tagfs_destroy (void *user_data)
{
    log_msg("SAVING TO DATABASE : %s\n", DB->db_fname);
    tagdb_save(DB, DB->db_fname);
    tagdb_destroy(DB);
    log_close();
    /* and now we're all clean */
    toggle_tagfs_consistency();
}

struct fuse_operations tagfs_oper = {
    .mknod = tagfs_mknod,
    .mkdir = tagfs_mkdir,
    .readlink = tagfs_readlink,
    .rmdir = tagfs_rmdir,
    .release = tagfs_release,
    //.opendir = tagfs_opendir,
    //.releasedir = tagfs_releasedir,
    .readdir = tagfs_readdir,
    .getattr = tagfs_getattr,
    .utime = tagfs_utime,
    .chmod = tagfs_chmod,
    .chown = tagfs_chown,
    //.access = tagfs_access,
    .unlink = tagfs_unlink,
    .rename = tagfs_rename,
    .create = tagfs_create,
    .ftruncate = tagfs_ftruncate,
    .truncate = tagfs_truncate,
    .flush = tagfs_flush,
    .open = tagfs_open,
    .read = tagfs_read,
    .write = tagfs_write,
    .destroy = tagfs_destroy,
};

void check_opt_arg (int position, int argc, char *argv[argc])
{
    if (position > argc)
    {
        fprintf(stderr, "must provide argument to %s\n", argv[position - 1]);
        abort();
    }
}

// The new argv has only fuse arguments
int proc_options (int argc, char *argv[argc], char *old_argv[argc],
        struct tagfs_state *data)
{
    int n = 0;
    int i;
    for (i = 0; i < argc; i++)
    {
        printf("argv[%d] = %s\n", i, old_argv[i]);
        if (g_strcmp0(old_argv[i], "-g") == 0
                || g_strcmp0(old_argv[i], "--debug") == 0)
        {
            data->debug = TRUE;
            i++;
            int log_filter = 0;
            if (isdigit(old_argv[i][0]))
            {
                log_filter = atoi(old_argv[i]);
            }
            else
            {
                i--;
            }
            log_open(data->log_file, log_filter);
            continue;
        }
        if (g_strcmp0(old_argv[i], "--no-debug") == 0)
        {
            data->debug = FALSE;
            continue;
        }
        argv[n] = old_argv[i];
        n++;
    }
    return n;
}

gboolean tagfs_is_consistent ()
{
    FILE *f = fopen(consistency_flag_file, "r");
    if (!f)
    {
        fopen(consistency_flag_file, "w");
        fputc('0', f);
        fclose(f);
        return FALSE;
    }
    return (fgetc(f) == '1' && fclose(f) == 0);
}

void toggle_tagfs_consistency ()
{
    printf("toggling consistency_flag_file\n");
    FILE *f = fopen(consistency_flag_file, "r+");
    int c = fgetc(f);
    rewind(f);
    if (c == '0')
    {
        printf("state 0\n");
        fputc('1', f);
    }
    else
    {
        printf("state 1\n");
        fputc('0', f);
    }
    fclose(f);
}

/* restores consistent state if we exited
   in error */
// TODO:fix this :(
void ensure_tagfs_consistency (struct tagfs_state *st)
{
    if (tagfs_is_consistent()) return;
    GList *files = get_files_list(st->db, "/");
    LL(files, it)
    {
        File *f = it->data;
        char *res = g_strdup_printf("%s/%ld", st->copiesdir, f->id);
        struct stat statbuf;
        if (lstat(res, &statbuf) == -1) delete_file(st->db, f);
        LL_END;
    }
    toggle_tagfs_consistency();
}

int main (int argc, char **argv)
{
    int fuse_stat = 0;
    struct tagfs_state *tagfs_data = g_try_malloc(sizeof(struct tagfs_state));
    if (!tagfs_data)
    {
        perror("Cannot alloc tagfs_data");
        abort();
    }
    char *prefix = g_build_filename(g_get_user_data_dir(), "tagfs", NULL);
    char *db_fname = g_build_filename(prefix, "tagfs.db", NULL);
    tagfs_data->log_file = g_build_filename(prefix, "tagfs.log", NULL);
    tagfs_data->copiesdir = g_build_filename(prefix, "copies", NULL);
    consistency_flag_file = g_build_filename(prefix, "tagfs.clean", NULL);
    tagfs_data->debug = FALSE;

    char *new_argv[argc];
    int new_argc = proc_options(argc, new_argv, argv, tagfs_data);

    if (mkdir(prefix, (mode_t) 0755) && errno != EEXIST)
        log_error("could not make data directory");
    if (mkdir(tagfs_data->copiesdir, (mode_t) 0755) && errno != EEXIST)
        log_error("could not make copies directory");
    if (new_argc != 2)
    {
        fprintf(stderr, "Must provide mount point for %s\n", new_argv[0]);
        abort();
    }

    log_msg("tagfs_data->copiesdir = \"%s\"\n", tagfs_data->copiesdir);
    log_msg("tagfs_data->mountdir = \"%s\"\n", tagfs_data->mountdir);

    char mountpath[PATH_MAX];
    tagfs_data->mountdir = realpath(new_argv[1], mountpath);

    if (!(tagfs_data->copiesdir && tagfs_data->mountdir))
    {
        log_msg("couldn't open required directories");
        abort();
    }
    tagfs_data->db = tagdb_load(db_fname);
    //ensure_tagfs_consistency(tagfs_data);
    /* at this point, we're guaranteed consistency.
       however, we know that they fs may change while mounted so we say that
       the system state is inconsistent at this point */
    //toggle_tagfs_consistency();

    tagfs_data->rqm = result_queue_manager_new();
    tagfs_data->stage = new_stage();

    tagfs_data->search_results = new_search_list();
    fprintf(stderr, "about to call fuse_main\n");
    fuse_stat = fuse_main(new_argc, new_argv, &tagfs_oper, tagfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    //g_free(consistency_flag_file);
    return fuse_stat;
}
