/* must be before fuse.h */
#include "tagfs.h"

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
#include "stage.h"
#include "tagdb.h"
#include "set_ops.h"
//#include "query.h"
#include "path_util.h"

static char *consistency_flag_file;

/*
   result_t *cmd_query (const char *query)
   {
   char *allowed_commands[] = {"TAG CREATE", "FILE SEARCH", "TAG REMOVE", "FILE ADD_TAGS", "FILE HAS_TAGS", NULL};
   int i;
   for (i = 0; allowed_commands[i] != NULL; i++)
   {
   if (g_str_has_prefix(query, allowed_commands[i]))
   {
   return tagdb_query(DB, query);
   }
   }
   return NULL;
   }
 */

int is_directory (const char *path)
{
    log_msg("is it a dir?\n");
    
    if (g_str_equal(path, "/")
            || g_str_has_suffix(path, UNTAG_FH))
            return TRUE;
    
    int res = 0;
    gulong *tags = path_extract_key(path);
    char *base = g_path_get_basename(path);

    char *dir = g_path_get_dirname(path);
    gulong *dirtags = path_extract_key(dir);

    if (tags)
    {
        if (g_strcmp0(dir, "/") == 0)
            res = 1;
        else if (stage_lookup(STAGE, dirtags, base))
            res = 1;
        else
        {
            GList *files = get_files_list(DB, path);
            if (files)
                res = 1;
            g_list_free(files);
        }
    }

    g_free(tags);
    g_free(base);
    g_free(dirtags);
    g_free(dir);
    return res;
}

int is_file (const char *path)
{
    log_msg("is it a file?\n");
    return (path_to_file(path) != NULL);
}

int tagfs_getattr (const char *path, struct stat *statbuf)
{
    log_msg("\ntagfs_getattr(path=\"%s\", statbuf=0x%08x)\n",
            path, statbuf);
    int retstat = -ENOENT; 
    if (is_directory(path))
    {
        log_msg("it's a directory!\n");
        statbuf->st_mode = S_IFDIR | 0755;
        retstat = 0;
    }
    else if (is_file(path))
    {
        char *fpath = get_file_copies_path(path);
        log_msg("it's a file!\n");
        retstat = lstat(fpath, statbuf);
        g_free(fpath);
    }
    else if (g_str_has_suffix(path, LISTEN_FH))
    {
        statbuf->st_mode = S_IFREG | 0755;
        statbuf->st_size = 0;
        retstat = 0;
    }
    else
    {
        char *base = g_path_get_basename(path);
        if (result_queue_exists(FSDATA->rqm, base))
        {
            statbuf->st_mode = S_IFREG | 0444;
            result_t *res = result_queue_peek(FSDATA->rqm, base);
            if (res != NULL)
            {
                char *str = tagdb_value_to_str(res);
                statbuf->st_size = strlen(str); /* get the size of the "file" */
                statbuf->st_blksize = 4096; /* based on other files values */
                g_free(str);
            }
            else
            {
                statbuf->st_size = 0;
            }
            retstat = 0;
        }
        g_free(base);
    }
    log_stat(statbuf);
    return retstat;
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
    _log_level = 0;
    log_msg("\ntagfs_rename(path=\"%s\", newpath=\"%s\")\n",
	    path, newpath);

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

            gulong *tags = path_extract_key(newdir);

            KL(tags, i)
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
            KL_END(tags, i);
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

int tagfs_create (const char *path, mode_t mode, struct fuse_file_info *fi)
{
    log_msg("\ntagfs_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n",
	    path, mode, fi);
    int retstat = 0;
    char *base = g_path_get_basename(path);
    char *dir = g_path_get_dirname(path);
    
    File *f = new_file(base);
    gulong *tags = path_extract_key(dir);
    if (!tags) 
    {
        errno = ENOENT;
        return -1;
    }
    KL(tags, i)
        add_tag_to_file(DB, f, tags[i], NULL);
    KL_END(tags, i);
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
    log_msg("\ntagfs_open(path\"%s\", f_info=0x%08x)\n",
	    path, f_info);

    char *fpath = get_file_copies_path(path);
    fd = open(fpath, f_info->flags);
    g_free(fpath);

    f_info->fh = fd;
    log_fi(f_info);
    return retstat;
}

int tagfs_write (const char *path, const char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi)
{
    _log_level = 0;
    log_msg("\ntagfs_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, f_info=0x%08x)\n",
	    path, buf, size, offset, fi);
    //log_msg("pid = %d\n", fuse_get_context()->pid);

    // check if we're writing to the "listen" file
    if (g_str_has_suffix(path, LISTEN_FH))
    {
        /*
        // sanitize the buffer string
        char *cmdstr = g_strstrip(g_memdup(buf, size + 1));
        cmdstr[size] = '\0';
        log_msg("query = %s\n", cmdstr);
        result_t *res = cmd_query(cmdstr);
        if (res != NULL)
        {
            log_msg("tagfs_write #LISTEN# tagdb_query result type=%d\
                    value=%s\n", res->type, tagdb_value_to_str((tagdb_value_t*) res));
            char *qrstr = g_strdup_printf("#QREAD-%d#", (int) fuse_get_context()->pid);
            if (!result_queue_exists(FSDATA->rqm, qrstr))
                result_queue_new(FSDATA->rqm, qrstr);
            if (res->type == tagdb_dict_t)
            {
                log_hash(res->data.d);
            }
            result_queue_add(FSDATA->rqm, qrstr, res);
            log_msg("tagfs_write rqm=%p\n", FSDATA->rqm);
            g_free(qrstr);
        }
        g_free(cmdstr);
        return size;
        */
        return 0;
    }
    return pwrite(fi->fh, buf, size, offset);
}

int tagfs_read (const char *path, char *buffer, size_t size, off_t offset,
        struct fuse_file_info *f_info)
{
    log_msg("\ntagfs_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, f_info=0x%08x)\n",
	    path, buffer, size, offset, f_info);
    int retstat = 0;
    char *base = g_path_get_basename(path);
    // check if we're writing to the "listen" file
    if (result_queue_exists(FSDATA->rqm, base))
    {
        result_t *res = result_queue_remove(FSDATA->rqm, base);
        if (!res)
            retstat = -1;
        else
        {
            char *result_string = tagdb_value_to_str((tagdb_value_t*) res);
            int reslen = strlen(result_string);
            int real_size = (reslen<=size)?reslen:size;
            memcpy(buffer, result_string, real_size);
            retstat = real_size;
        }
        result_destroy(res);
        res = NULL;
    }
    // no need to get fpath on this one, since I work from f_info->fh not the path
    log_fi(f_info);

    retstat = pread(f_info->fh, buffer, size, offset);
    g_free(base);
    return retstat;
}

int tagfs_flush(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\ntagfs_flush(path=\"%s\", fi=0x%08x)\n",
	    path, fi);
    
    return retstat;
}

int tagfs_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\ntagfs_fsync(path=\"%s\", datasync=%d, fi=0x%08x)\n",
	    path, datasync, fi);
    log_fi(fi);
    
#ifndef __HAIKU__
    if (datasync)
        retstat = fdatasync(fi->fh);
    else
#endif
        retstat = fsync(fi->fh);
    
    if (retstat < 0)
        log_error("tagfs_fsync fsync");
    
    return retstat;
}

int tagfs_truncate(const char *path, off_t newsize)
{
    int retstat = 0;

    log_msg("\ntagfs_truncate(path=\"%s\", newsize=%lld)\n",
	    path, newsize);
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
    log_msg("\ntagfs_ftruncate(path=\"%s\", offset=%lld, fi=0x%08x)\n",
	    path, offset, fi);
    
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
    log_msg("\ntagfs_mknod (path=\"%s\", mode=0%3o, dev=%lld)\n",
	  path, mode, dev);

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
    
    g_free(base);
    g_free(fpath);
    return retstat;
}

int tagfs_mkdir (const char *path, mode_t mode)
{
    log_msg("\ntagfs_mkdir (path=\"%s\")\n",
	  path);
    char *base = g_path_get_basename(path);
    char *dir = g_path_get_dirname(path);
    gulong *key = path_extract_key(dir);

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

int tagfs_unlink (const char *path)
{
    log_msg("\ntagfs_unlink (path=\"%s\")\n", path);
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
    log_msg("\ntagfs_release(path=\"%s\", fi=0x%08x)\n",
	  path, f_info);
    log_fi(f_info);
    if (g_str_has_suffix(path, LISTEN_FH))
        return 0;

    return close(f_info->fh);
}

int tagfs_readdir (const char *path, void *buffer, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *f_info)
{
    log_msg("\ntagfs_readdir(path=\"%s\", buffer=0x%08x, filler=0x%08x, offset=%lld, f_info=0x%08x)\n",
	    path, buffer, filler, offset, f_info);

    //char *dir = g_path_get_dirname(path);
    int retstat = 0;
    
    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);

    gulong *tags = path_extract_key(path);
    GList *f = get_files_list(DB, path);
    GList *t = NULL;

    if (g_strcmp0(path, "/") == 0)
        t = g_hash_table_get_values(DB->tags);
    else
        t = get_tags_list(DB, path);

    GList *s = stage_list_position(STAGE, tags);
    s = g_list_sort(s, (GCompareFunc) file_name_cmp);
    t = g_list_sort(t, (GCompareFunc) file_name_cmp);

    GList *combined_tags = g_list_union(s, t, (GCompareFunc) file_name_cmp);

    LL(f, it)
        if (filler(buffer, file_to_string(it->data), NULL, 0))
        {
            log_msg("    ERROR tagfs_readdir filler:  buffer full");
            return -1;
        }
    LL_END(it);

    LL(combined_tags, it)
        if (filler(buffer, tag_to_string(it->data), NULL, 0))
        {
            log_msg("    ERROR tagfs_readdir filler:  buffer full");
            return -1;
        }
    LL_END(it);

    g_free(tags);
    g_list_free(f);
    g_list_free(t);
    g_list_free(s);
    g_list_free(combined_tags);

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
    errno = EPERM;
    return -1;
}

int tagfs_utime(const char *path, struct utimbuf *ubuf)
{
    int retstat = 0;
    
    log_msg("\ntagfs_utime(path=\"%s\", ubuf=0x%08x)\n",
	    path, ubuf);

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
    log_msg("\ntagfs_access(path=\"%s\", mask=0%o)\n",
	    path, mask);
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
    
    log_msg("\ntagfs_chmod(fpath=\"%s\", mode=0%03o)\n",
	    path, mode);
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
    
    log_msg("\ntagfs_chown(path=\"%s\", uid=%d, gid=%d)\n",
	    path, uid, gid);
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
    .access = tagfs_access,
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
void ensure_tagfs_consistency (struct tagfs_state *st)
{
    if (tagfs_is_consistent()) return;
    gulong key[] = {0, 0, 0};
    GList *files = get_files_list(st->db, key);
    LL(files, it)
        File *f = it->data;
        char *res = g_strdup_printf("%s/%ld", st->copiesdir, f->id);
        struct stat statbuf;
        if (lstat(res, &statbuf) == -1) delete_file(st->db, f);
    LL_END(it);
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
    ensure_tagfs_consistency(tagfs_data);
    /* at this point, we're guaranteed consistency.
       however, we know that they fs may change while mounted so we say that
       the system state is inconsistent at this point */
    toggle_tagfs_consistency();

    tagfs_data->rqm = malloc(sizeof(ResultQueueManager));
    tagfs_data->rqm->queue_table = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify) g_free, 
            (GDestroyNotify) g_queue_free);

    tagfs_data->stage = new_stage();

    fprintf(stderr, "about to call fuse_main\n");
    fuse_stat = fuse_main(new_argc, new_argv, &tagfs_oper, tagfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    //g_free(consistency_flag_file);
    return fuse_stat;
}
