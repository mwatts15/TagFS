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
#include "set_ops.h"
#include "result_queue.h"
#include "tagdb.h"
#include "query.h"
#include "path_util.h"

result_t *cmd_query (const char *query)
{
    char *allowed_commands[] = {"TAG CREATE", "TAG TSPEC", "TAG REMOVE", "FILE ADD_TAGS", "FILE HAS_TAGS", NULL};
    int i;
    for (i = 0; allowed_commands[i] != NULL; i++)
    {
        if (g_str_has_prefix(query, allowed_commands[i]))
        {
            return tagdb_query(TAGFS_DATA->db, query);
        }
    }
    return NULL;
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
    if (g_strcmp0(path, "/") == 0)
    {
        statbuf->st_mode = S_IFDIR | 0755;
        retstat = 0;
    }
    else
    {
        char *fpath = get_id_copies_path(path);
        if (fpath != NULL)
        {
            retstat = lstat(fpath, statbuf);
        }
        else if (g_str_has_suffix(path, TAGFS_DATA->listen))
        {
            retstat = 0;
            statbuf->st_mode = S_IFREG | 0755;
            statbuf->st_size = 0;
        }
        else
        {
            char *qstring = path_to_qstring(path, FALSE);
            result_t *res = tagdb_query(TAGFS_DATA->db, qstring);
            if (res != NULL && res->type == tagdb_dict_t && res->data.d != NULL)
            {
                retstat = 0;
                statbuf->st_mode = S_IFDIR | 0755;
            }
            else
            {
                char *basecopy = g_strdup(path);
                char *base = basename(basecopy);
                if (result_queue_exists(TAGFS_DATA->rqm, base))
                {
                    retstat = 0;
                    statbuf->st_mode = S_IFREG | 0444;
                    result_t *res = result_queue_peek(TAGFS_DATA->rqm, base);
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
                }
                g_free(basecopy);
            }
            g_free(qstring);
            result_destroy(res);
        }
        g_free(fpath);
    }
    log_stat(statbuf);
    return retstat;
}

int tagfs_readlink (const char *path, char *realpath, size_t bufsize)
{
    char *copies_path = get_id_copies_path(path);
    int retstat = readlink(copies_path, realpath, bufsize);
    g_free(copies_path);
    return retstat;
}

int tagfs_rename (const char *path, const char *newpath)
{
    log_msg("\ntagfs_rename(path=\"%s\", newpath=\"%s\")\n",
	    path, newpath);

    int retstat = 0;
    char *qstring = NULL;

    char *base = g_path_get_basename(path);
    char *newbase = g_path_get_basename(newpath);

    int tag_id = tagdb_get_tag_code(TAGFS_DATA->db, base);
    if (tag_id > 0)
    {
        qstring = g_strdup_printf("TAG RENAME \"%s\" \"%s\"", base, newbase);
    }
    else
    {
        int file_id = path_to_file_id(path);
        
        if (file_id > 0)
        {
            char *dir = g_path_get_dirname(newpath);
            char *tags_to_add = path_to_tags(dir);

            _log_level = 0;
            qstring = g_strdup_printf("FILE ADD_TAGS %d %s name:%s", 
                    file_id, tags_to_add, newbase);
            g_free(dir);
            g_free(tags_to_add);
        }
        else
        {
            log_error("tagfs_rename file_id <= 0\n");
        }
    }

    result_t *res = tagdb_query(TAGFS_DATA->db, qstring);
    if (res->type == tagdb_err_t)
    {
        retstat = -1;
        log_error("tagfs_rename");
    }

    result_destroy(res);
    g_free(qstring);
    g_free(newbase);
    g_free(base);
    qstring = NULL;
    return retstat;
}

int tagfs_create (const char *path, mode_t mode, struct fuse_file_info *fi)
{
    int retstat = 0;
    char *base = g_path_get_basename(path);
    char *dir = g_path_get_dirname(path);

    log_msg("\ntagfs_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n",
	    path, mode, fi);
    
    char *tags = path_to_tags(dir);
    char *qstring = g_strdup_printf("FILE CREATE name:\"%s\" %s", base, tags);

    result_t *res = tagdb_query(TAGFS_DATA->db, qstring);

    if (res->type == tagdb_int_t && res->data.i != 0)
    {
        int maxlen = 16;
        char id_string[maxlen];

        g_snprintf(id_string, maxlen, "%d", res->data.i);

        char *fpath = tagfs_realpath(id_string);
        int fd = creat(fpath, mode);

        fi->fh = fd;

        g_free(fpath);
    }
    else
    {
        retstat = -1;
    }

    g_free(base);
    g_free(dir);
    g_free(tags);
    g_free(qstring);
    return retstat;
}

int tagfs_open (const char *path, struct fuse_file_info *f_info)
{
    int retstat = 0;
    int fd;
    log_msg("\ntagfs_open(path\"%s\", f_info=0x%08x)\n",
	    path, f_info);

    char *fpath = get_id_copies_path(path);
    fd = open(fpath, f_info->flags);
    g_free(fpath);

    f_info->fh = fd;
    log_fi(f_info);
    return retstat;
}

int tagfs_write (const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    log_msg("\ntagfs_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, f_info=0x%08x)\n",
	    path, buf, size, offset, fi);
    log_msg("pid = %d\n", fuse_get_context()->pid);

    // check if we're writing to the "listen" file
    if (g_str_has_suffix(path, TAGFS_DATA->listen))
    {
        // sanitize the buffer string
        char *cmdstr = g_strstrip(g_memdup(buf, size + 1));
        cmdstr[size] = '\0';
        result_t *res = cmd_query(cmdstr);
        log_msg("query = %s\n", cmdstr);
        if (res != NULL)
        {
            log_msg("tagfs_write #LISTEN# tagdb_query result type=%d\
                    value=%s\n", res->type, tagdb_value_to_str((tagdb_value_t*) res));
            char *qrstr = g_strdup_printf("#QREAD-%d#", (int) fuse_get_context()->pid);
            if (!result_queue_exists(TAGFS_DATA->rqm, qrstr))
                result_queue_new(TAGFS_DATA->rqm, qrstr);
            if (res->type == tagdb_dict_t)
            {
                log_hash(res->data.d);
            }
            result_queue_add(TAGFS_DATA->rqm, qrstr, res);
            log_msg("tagfs_write rqm=%p\n", TAGFS_DATA->rqm);
            g_free(qrstr);
        }
        g_free(cmdstr);
        return size;
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
    if (result_queue_exists(TAGFS_DATA->rqm, base))
    {
        result_t *res = result_queue_remove(TAGFS_DATA->rqm, base);
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

// no clue why this is needed, but whatever
int tagfs_flush(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\ntagfs_flush(path=\"%s\", fi=0x%08x)\n",
	    path, fi);
    
    return retstat;
}

int tagfs_truncate(const char *path, off_t newsize)
{
    int retstat = 0;

    log_msg("\ntagfs_truncate(path=\"%s\", newsize=%lld)\n",
	    path, newsize);
    char *fpath = get_id_copies_path(path);
    
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

// Create a tag
void create_tag (const char *tag)
{
    if (TAGFS_DATA == NULL)
    {
        fprintf(stderr, "Must initialize fuse first\n");
    }
    tagdb_insert_item(TAGFS_DATA->db, g_strdup(tag), NULL, TAG_TABLE);
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
    char *base = NULL;
    char *basecopy = NULL;
    char *fpath = NULL;
    result_t *res = NULL;
    char *qstring = NULL;
    //int fd;
    int retstat;
    int maxlen = 16;
    char id_string[maxlen];
    basecopy = g_strdup(path);
    base = basename(basecopy);

    log_msg("\ntagfs_mknod (path=\"%s\", mode=0%3o, dev=%lld)\n",
	  path, mode, dev);
    
    qstring = g_strdup_printf("FILE CREATE name:%s", base);
    res = tagdb_query(TAGFS_DATA->db, qstring);

    if (res->type == tagdb_int_t)
        g_snprintf(id_string, maxlen, "%d", res->data.i);

    fpath = tagfs_realpath(id_string);
    if (S_ISREG(mode)) {
        retstat = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (retstat >= 0)
            retstat = close(retstat);
    } 
    else if (S_ISFIFO(mode)) {
	    retstat = mkfifo(fpath, mode);
	} else {
	    retstat = mknod(fpath, mode, dev);
	}
    
    g_free(basecopy);
    g_free(fpath);
    return retstat;
}

int tagfs_mkdir (const char *path, mode_t mode)
{
    char *basecopy = g_strdup(path);
    char *base = basename(basecopy);

    char *qstring = g_strdup_printf("TAG CREATE \"%s\" INT", base);
    log_msg(qstring);
    tagdb_value_t *t = tagdb_query(TAGFS_DATA->db, qstring);
    result_destroy(t);
    g_free(qstring);
    g_free(basecopy);
    return 0;
}

// remove the file for real and remove from the
// db structure
int tagfs_unlink (const char *path)
{
    log_msg("\ntagfs_unlink (path=\"%s\", mode=0%3o, dev=%lld)\n",
	  path);
    int retstat = 0;
    char *fpath = NULL;
    // get the file's id
    // since there isn't a unique "name" for a file
    // we have to get the intersection of the files
    // from our current query and the "name" tag.
    char *qstring = path_to_qstring(path, TRUE);
    result_t *res = tagdb_query(TAGFS_DATA->db, qstring);
    g_free(qstring);

    if (res->type == tagdb_err_t)
        return -1; // might do something more sophisticated later

    if (res->type == tagdb_dict_t)
    {
        int max = 0;
        int maxlen = 16; // maximum length of an id
        GHashTableIter it;
        gpointer k, v;

        g_hash_loop(res->data.d, it, k, v)
        {
            max = GPOINTER_TO_INT(k);
            break;
        }
        qstring = g_strdup_printf("FILE REMOVE %d", max);
        tagdb_query(TAGFS_DATA->db, qstring);
        g_free(qstring);
        char id_string[maxlen];
        g_snprintf(id_string, maxlen, "%d", max);
        fpath = tagfs_realpath(id_string);
        retstat = unlink(fpath);
    }
    g_free(fpath);
    return retstat;
}

int tagfs_release (const char *path, struct fuse_file_info *f_info)
{
    log_msg("\ntagfs_release(path=\"%s\", fi=0x%08x)\n",
	  path, f_info);
    log_fi(f_info);
    if (g_str_has_suffix(path, TAGFS_DATA->listen))
        return 0;

    return close(f_info->fh);
}

// Get the tree node from the path
// get the files with those tags and add them in
// get the children of the node and add them in
// get the tags the files have that aren't already
//  in the path
int tagfs_readdir (const char *path, void *buffer, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *f_info)
{
    log_msg("\ntagfs_readdir(path=\"%s\", buffer=0x%08x, filler=0x%08x, offset=%lld, f_info=0x%08x)\n",
	    path, buffer, filler, offset, f_info);
    /*
    struct stat statbuf;
    int stat = lstat(TAGFS_DATA->mountdir, &statbuf);
    log_msg("lstat returns %d for %s\n", stat, TAGFS_DATA->mountdir);
    */
    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);
    char *qstring = path_to_qstring(path, FALSE);
    result_t *res = tagdb_query(TAGFS_DATA->db, qstring);
    g_free(qstring);
    qstring = NULL;

    if (res->type == tagdb_dict_t)
    {
        GHashTableIter it;
        gpointer k, v;
        int dircount = 0;
        tagdb_value_t *freeme = NULL;
        g_hash_loop(res->data.d, it, k, v)
        {
            // convert the id to a string if we don't have a name and use that
            int namecode = tagdb_get_tag_code(TAGFS_DATA->db, "name");
            tagdb_value_t *name = tagdb_get_sub(TAGFS_DATA->db, GPOINTER_TO_INT(k), namecode, FILE_TABLE);
            if (name == NULL)
            {
                freeme = tagdb_str_to_value(tagdb_str_t, "!NO_NAME!");
                name = freeme;
            }
            _log_level++;
            log_msg("calling filler with name %s\n", name->data.s);
            _log_level--;
            if (filler(buffer, name->data.s, NULL, 0) != 0)
            {
                // might need something to free
                // the result_t...
                //g_hash_table_unref(res->data.d);
                g_free(res);
                log_msg("    ERROR tagfs_readdir filler:  buffer full");
                return -errno;
            }
            g_free(freeme);
            dircount++;
        }

        if (dircount == 1)
        {
            return 0;
        }
        // we take the union of tags from the files too
        GList *all_tags;
        GHashTable *tag_table;
        if (g_strcmp0(path, "/") == 0)
            tag_table = tagdb_get_table(TAGFS_DATA->db, TAG_TABLE);
        else
        {
            all_tags = g_hash_table_get_values(res->data.d);
            tag_table = set_union(all_tags);
            g_list_free(all_tags);
        }
        g_free(res);
        g_hash_loop(tag_table, it, k, v)
        {
            char *tagname = tagdb_get_tag_value(TAGFS_DATA->db, TO_I(k));
            if (filler(buffer, tagname, NULL, 0) != 0)
            {
                // might need something to free
                // the result_t...
                //g_hash_table_unref(res->data.d);
                log_msg("    ERROR tagfs_readdir filler:  buffer full");
                return -errno;
            }
            dircount++;
        }
    }
    //g_free(res->data.b);
    return 0;
}

// This causes problems because the directory
// "contains" entries
// we just return EPERM
int tagfs_rmdir (const char *path)
{
    /*
    char *basec = NULL;
    char *base = NULL;
    basec = g_strdup(path);
    base = basename(basec);

    char *qstring = g_strdup_printf("TAG REMOVE %s", base);
    tagdb_query(TAGFS_DATA->db, qstring);
    */
    errno = -EPERM;
    return -1;
}

void tagfs_destroy (void *user_data)
{
    tagdb_save(TAGFS_DATA->db, NULL, NULL);
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

// The new argv has only fuse arguments
int proc_options (int argc, char *argv[argc], char *old_argv[argc],
        struct tagfs_state *data)
{
    int n = 1;
    int i;
    for (i = 1; i < argc; i++)
    {
        if (g_strcmp0(old_argv[i], "-g") == 0 
                || g_strcmp0(old_argv[i], "--debug") == 0)
        {
            data->debug = TRUE;
            log_open("tagfs.log", 0);
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
    tagfs_data = calloc(sizeof(struct tagfs_state), 1);
    if (tagfs_data == NULL)
    {
        perror("Cannot alloc tagfs_data");
        abort();
    }
    tagfs_data->debug = FALSE;

    char *new_argv[argc];
    int new_argc = proc_options(argc, new_argv, argv, tagfs_data);
    tagfs_data->db = newdb("test.db", "test.types");
    if (new_argc != 3) abort(); // program name + copies + mount
    char copiespath[PATH_MAX];
    char mountpath[PATH_MAX];
    tagfs_data->copiesdir = realpath(new_argv[1], copiespath);
    tagfs_data->mountdir = realpath(new_argv[2], mountpath);
    printf("tagfs_data->copiesdir = \"%s\"\n", tagfs_data->copiesdir);
    printf("tagfs_data->mountdir = \"%s\"\n", tagfs_data->mountdir);
    if (tagfs_data->copiesdir == NULL || tagfs_data->mountdir == NULL) abort();
    tagfs_data->listen = "#LISTEN#";
    tagfs_data->rqm = malloc(sizeof(ResultQueueManager));
    tagfs_data->rqm->queue_table = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify) g_free, 
            (GDestroyNotify) g_queue_free);
    new_argv[1] = new_argv[2];
    new_argc--;

    fprintf(stderr, "about to call fuse_main\n");
    fuse_stat = fuse_main(new_argc, new_argv, &tagfs_oper, tagfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    return fuse_stat;
}
