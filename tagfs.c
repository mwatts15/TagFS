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
//#include "set_ops.h"
#include "result_queue.h"
#include "tagdb.h"
//#include "query.h"
#include "path_util.h"

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
    if (g_strcmp0(path, "/") == 0) return TRUE;
    gulong *tags = translate_path(path);
    if (tags == NULL)
        return FALSE;
    g_free(tags);
    return TRUE;
}

int is_file (const char *path)
{
    log_msg("is it a file?\n");
    return (path_to_file(path) != NULL);
}
// all dirs have the same permissions as the
// mount dir.
// a file is a dir if i can't find a file that matches
// that path
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
    else if (g_str_has_suffix(path, FSDATA->listen))
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
            gulong *tags = translate_path(newdir);
            set_file_name(f, newbase, DB);
            KL(tags, i)
                add_tag_to_file(DB, f, tags[i], NULL);
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
    gulong *tags = translate_path(dir);
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
    if (g_str_has_suffix(path, FSDATA->listen))
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
    char *basecopy = g_strdup(path);
    char *base = basename(basecopy);
    int retstat = 0;
    // check if we're writing to the "listen" file
    if (result_queue_exists(FSDATA->rqm, base))
    {
        result_t *res = result_queue_remove(FSDATA->rqm, base);
        if (res == NULL)
            return -1;
        char *result_string = tagdb_value_to_str((tagdb_value_t*) res);
        int reslen = strlen(result_string);
        int real_size = (reslen<=size)?reslen:size;
        memcpy(buffer, result_string, real_size);
        result_destroy(res);
        res = NULL;
        return real_size;
    }
    // no need to get fpath on this one, since I work from f_info->fh not the path
    log_fi(f_info);

    retstat = pread(f_info->fh, buffer, size, offset);
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

// create the real file with a new id
// tag it with the given name
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

    Tag *t = lookup_tag(DB, base);
    if (t == NULL)
    {
        // Make a new tag object
        // give it int type with default value 0
        t = new_tag(base, tagdb_int_t, 0);
        // Insert it into the TagDB TagTree
        insert_tag(DB, t);
    }
    // Add to staging table

    g_free(base);
    return 0;
}

int tagfs_unlink (const char *path)
{
    log_msg("\ntagfs_unlink (path=\"%s\")\n",
	  path);
    int retstat = 0;
    char *dir = g_path_get_dirname(path);
    char *base = g_path_get_basename(path);

    gulong *tags = translate_path(dir);
    File *f = retrieve_file(DB, tags, base);

    if(f)
    {
        /* we want to skip the first entry in tags
           so that all files (with unique names) show up here
           except when you delete the file in the root directory
           in which case it should do what's intended */
        file_cabinet_remove_v(DB, tags + 1, f);
    }
    else
    {
        retstat = -1;
        errno = ENOENT;
    }

    char *fpath;
    /* this idiom is really provided as a sort of convenience to users
       so they don't have to track the file down with every tag to delete
       it properly. But this also ensures that we aren't storing up files
       that can't be accessed both in the database and in the copies
       directory */
    if (g_str_equal(dir, "/") && (fpath = tagfs_realpath(f)))
    {
        retstat = unlink(fpath);
        delete_file(DB, f);
        g_free(fpath);
    }
    
    g_free(tags);
    return retstat;
}

int tagfs_release (const char *path, struct fuse_file_info *f_info)
{
    log_msg("\ntagfs_release(path=\"%s\", fi=0x%08x)\n",
	  path, f_info);
    log_fi(f_info);
    if (g_str_has_suffix(path, FSDATA->listen))
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

    gulong *tags = translate_path(path);
    GList *f = get_files_list(DB, tags);
    GList *t = get_tags_list(DB, tags, f);
    // get list of staged tags

    LL(f, it)
        if (filler(buffer, file_to_string(it->data), NULL, 0))
        {
            log_msg("    ERROR tagfs_readdir filler:  buffer full");
            return -1;
        }
    LL_END(it);

    LL(t, it)
        if (filler(buffer, tag_to_string(it->data), NULL, 0))
        {
            log_msg("    ERROR tagfs_readdir filler:  buffer full");
            return -1;
        }
    LL_END(it);

    // add in staged tags

    g_list_free(f);
    g_list_free(t);

    log_msg("leaving readdir\n");
    return retstat;
}

int tagfs_rmdir (const char *path)
{
    /* Every file tagged with base name tag will have all tags in the
       dirname of the path removed from it.

       Essentially, it's what would happen if you called unlink on all of
       the files within seperately */
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
    int retstat = 0;
   
    log_msg("\ntagfs_access(path=\"%s\", mask=0%o)\n",
	    path, mask);
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
            log_open("tagfs.log", log_filter);
            continue;
        }
        if (g_strcmp0(old_argv[i], "--no-debug") == 0)
        {
            data->debug = FALSE;
            continue;
        }
        if (g_strcmp0(old_argv[i], "-d") == 0)
        {
            i++;
            printf("%s\n", old_argv[i]);
            check_opt_arg(i, argc, old_argv);
            data->copiesdir = realpath(old_argv[i], NULL);
            continue;
        }
        argv[n] = old_argv[i];
        n++;
    }
    return n;
}

int main (int argc, char **argv)
{
    int fuse_stat;
    struct tagfs_state *tagfs_data = NULL;

    // bbfs doesn't do any access checking on its own (the comment
    // blocks in fuse.h mention some of the functions that need
    // accesses checked -- but note there are other functions, like
    // chown(), that also need checking!).  Since running bbfs as root
    // will therefore open Metrodome-sized holes in the system
    // security, we'll check if root is trying to mount the filesystem
    // and refuse if it is.  The somewhat smaller hole of an ordinary
    // user doing it with the allow_other flag is still there because
    // I don't want to parse the options string.
    if ((getuid() == 0) || (geteuid() == 0)) {
        fprintf(stderr, "Running TagFS as root opens unnacceptable security holes\n");
        return 1;
    }
    tagfs_data = calloc(1, sizeof(struct tagfs_state));
    if (tagfs_data == NULL)
    {
        perror("Cannot alloc tagfs_data");
        abort();
    }
    tagfs_data->debug = FALSE;

    char *new_argv[argc];
    int new_argc = proc_options(argc, new_argv, argv, tagfs_data);
    
    tagfs_data->db = tagdb_load("test.db");
    
    if (new_argc != 2) 
    {
        fprintf(stderr, "Must provide mount point for %s\n", new_argv[0]);
        abort(); // program name + mount
    }

    char mountpath[PATH_MAX];
    tagfs_data->mountdir = realpath(new_argv[1], mountpath);

    printf("tagfs_data->copiesdir = \"%s\"\n", tagfs_data->copiesdir);
    printf("tagfs_data->mountdir = \"%s\"\n", tagfs_data->mountdir);

    if (tagfs_data->copiesdir == NULL || tagfs_data->mountdir == NULL) abort();
    tagfs_data->listen = "#LISTEN#";
    tagfs_data->rqm = malloc(sizeof(ResultQueueManager));
    tagfs_data->rqm->queue_table = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify) g_free, 
            (GDestroyNotify) g_queue_free);

    fprintf(stderr, "about to call fuse_main\n");
    fuse_stat = fuse_main(new_argc, new_argv, &tagfs_oper, tagfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    return fuse_stat;
}
