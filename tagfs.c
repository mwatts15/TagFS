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

// returns the file in our copies directory corresponding to
// the one in path
// should only be called on regular files since
// directories are only virtual
static char *tagfs_realpath(const char *path)
{
    char *res = g_strconcat(TAGFS_DATA->copiesdir, "/", path, NULL);
    log_msg("tagfs_realpath(path=\"%s\") = \"%s\"\n", path, res);
    return res;
}

static int tagfs_error(char *str)
{
    int ret = -errno;

    log_msg("    ERROR %s: %s\n", str, strerror(errno));

    return ret;
}

char *path_to_qstring (const char *path, gboolean is_file_path)
{
    char *qstring = NULL;

    if (is_file_path)
    {
        char *basecopy = g_strdup(path);
        char *base = basename(basecopy);
        char *dircopy = g_strdup(path);
        char *dir = dirname(dircopy);
        if (g_strcmp0(dir, "/") == 0)
            qstring = g_strdup_printf( "TAG TSPEC %sname=%s", dir, base);
        else
            qstring = g_strdup_printf( "TAG TSPEC %s/name=%s", dir, base);
        g_free(basecopy);
        g_free(dircopy);
    }
    else
    {
        qstring = g_strdup_printf( "TAG TSPEC %s", path);
    }
    log_msg("path_to_qstring, qstring=%s\n", qstring);
    return qstring;
}

void cmd_query (const char *query)
{
    char *allowed_commands[4] = {"TAG CREATE", "TAG REMOVE", "FILE ADD_TAGS", "FILE HAS_TAGS"};
    int i;
    for (i = 0; i < 4; i++)
    {
        if (g_str_has_prefix(query, allowed_commands[i]))
        {
            result_t *res = tagdb_query(TAGFS_DATA->db, query);
            char *str_res = tagdb_value_to_str(res->type, &(res->data));
            // Add to the queue
            // I think this will cause the thread to hang while
            // waiting for the client to read, but it should be okay
        }
    }
}

// turn the path into a file in the copies directory
// path_to_qstring + tagdb_query + tagfs_realpath
// NULL for a file that DNE
char *get_id_copies_path (const char *path)
{
    int id = path_to_file_id(path);
    if (id == 0)
        return NULL;
    int maxlen = 16;
    char id_string[maxlen];
    int length = g_snprintf(id_string, maxlen, "%d", id);
    if (length >= maxlen)
    {
        log_msg("get_id_copies_path: id (%d) too long\n", id);
        exit(-1);
    }
    return tagfs_realpath(id_string);
}

int path_to_file_id (const char *path)
{
    char *qstring = path_to_qstring(path, TRUE);
    result_t *res = tagdb_query(TAGFS_DATA->db, qstring);
    g_free(qstring);
    if (res == NULL)
    {
        log_msg("path_to_file_id got res==NULL\n");
        return 0;
    }

    if (res->type == tagdb_dict_t)
    {
        if (g_hash_table_size(res->data.d) != 0)
        {
            gpointer k, v;
            GHashTableIter it;
            g_hash_loop(res->data.d, it, k, v)
            {
                g_free(res);
                return TO_I(k);
            }
        } 
        else
        {
            g_free(res);
            return 0;
        }
    }
}

// all dirs have the same permissions as the
// mount dir.
// a file is a dir if i can't find a file that matches
// that path
int tagfs_getattr (const char *path, struct stat *statbuf)
{
    int retstat = 0;
    char *fpath = NULL;
    char *qstring = NULL;
    result_t *res = NULL;

    log_msg("\ntagfs_getattr(path=\"%s\", statbuf=0x%08x)\n",
	  path, statbuf);

    memset(statbuf, 0, sizeof(statbuf));
    statbuf->st_uid = fuse_get_context()->uid;
    statbuf->st_gid = fuse_get_context()->gid;
    if (g_strcmp0(path, "/") == 0)
    {
        //lstat(TAGFS_DATA->mountdir, statbuf);
        statbuf->st_mode = S_IFDIR | 0755;
        return retstat;
    }

    // well, does a query return anything?
    // if so we'll take it
    qstring = path_to_qstring(path, FALSE);
    log_msg("getattr_test\n");
    res = tagdb_query(TAGFS_DATA->db, qstring);
    g_free(qstring);
    if (res != NULL && res->type == tagdb_dict_t && g_hash_table_size(res->data.d) > 0)
    {
        statbuf->st_mode = S_IFDIR | 0755;
        g_free(res);
        res = NULL;
        return retstat;
    }
    else if (res->type == tagdb_err_t)
    {
        log_msg("error in getattr query: %s\n", res->data.s);
    }
    else
    {
        g_free(res);
        res = NULL;
    }

    if (g_str_has_suffix(path, TAGFS_DATA->listen))
    {
        statbuf->st_mode = S_IFREG | 0755;
        statbuf->st_size = 0;
        log_stat(statbuf);
        return retstat;
    }

    char *basecopy = g_strdup(path);
    char *base = basename(basecopy);

    if (result_queue_exists(TAGFS_DATA->rqm, base))
    {
        statbuf->st_mode = S_IFREG | 0444;
        // get the size of the "file"
        result_t *res = result_queue_peek(TAGFS_DATA->rqm, base);
        if (res != NULL)
        {
            char *str = tagdb_value_to_str(res->type, &(res->data));
            statbuf->st_size = strlen(str);
            statbuf->st_blksize = 4096;
            g_free(str);
        }
        else
        {
            statbuf->st_size = 0;
        }
        g_free(basecopy);
        log_stat(statbuf);
        return retstat;
    }
    g_free(basecopy);
    basecopy = NULL;

    // Check if it's a file
    fpath = get_id_copies_path(path);
    if (fpath != NULL)
    {
        retstat = lstat(fpath, statbuf);
        g_free(fpath);
        log_stat(statbuf);
        return retstat;
    }
    return -ENOENT;
}

int tagfs_rename (const char *path, const char *newpath)
{
    log_msg("\ntagfs_rename(path=\"%s\", newpath=\"%s\")\n",
	    path, newpath);
    int retstat = 0;
    int file_id = 0;
    int tag_id = 0;
    char *qstring = NULL;
    char *basec = NULL;
    char *newbasec = NULL;
    char *base = NULL;
    char *newbase = NULL;

    basec = g_strdup(path);
    base = basename(basec);

    newbasec = g_strdup(newpath);
    newbase = basename(newbasec);

    log_msg("tagfs_rename tag name = %s\n", base);
    tag_id = tagdb_get_tag_code(TAGFS_DATA->db, base);

    if (tag_id > 0)
    {
        qstring = g_strdup_printf("TAG RENAME %s %s", base, newbase);
    }
    else
    {
        file_id = path_to_file_id(path);
        if (file_id > 0)
            qstring = g_strdup_printf("FILE ADD_TAGS %d name:%s", 
                    file_id, newbase);
        else
        {
            log_msg("file id <= 0\n");
            qstring = NULL;
        }
    }
    result_t *res = tagdb_query(TAGFS_DATA->db, qstring);
    g_free(qstring);
    qstring = NULL;
    if (res->type == tagdb_err_t)
    {
        retstat = -1;
        tagfs_error("tagfs_rename");
    }
    //result_destroy(res);
    return retstat;
}

int tagfs_create (const char *path, mode_t mode, struct fuse_file_info *fi)
{
    char *base = NULL;
    char *basecopy = NULL;
    char *fpath = NULL;
    result_t *res = NULL;
    char *qstring = NULL;
    int fd;
    int retstat;
    int maxlen = 16;
    char id_string[maxlen];
    basecopy = g_strdup(path);
    base = basename(basecopy);

    log_msg("\ntagfs_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n",
	    path, mode, fi);
    
    qstring = g_strdup_printf("FILE CREATE name:%s", base);
    res = tagdb_query(TAGFS_DATA->db, qstring);

    if (res->type == tagdb_int_t)
        g_snprintf(id_string, maxlen, "%d", res->data.i);
    fpath = tagfs_realpath(id_string);
    fd = creat(fpath, mode);
    fi->fh = fd;

    g_free(basecopy);
    g_free(fpath);
    return 0;
}

int tagfs_open (const char *path, struct fuse_file_info *f_info)
{
    int retstat = 0;
    int fd;
    log_msg("\ntagfs_open(path\"%s\", f_info=0x%08x)\n",
	    path, f_info);

    if (g_str_has_suffix(path, TAGFS_DATA->listen))
    {
        char tmp_path[PATH_MAX];
        g_snprintf(tmp_path, PATH_MAX, "/tmp/%d-TagFS-LISTEN", fuse_get_context()->uid);
        fd = open(tmp_path, O_WRONLY);
        f_info->fh = fd;
        return retstat;
    }

    char *basec = g_strdup(path);
    char *base = basename(basec);
    if (result_queue_exists(TAGFS_DATA->rqm, base))
    {
        char tmp_path[PATH_MAX];
        g_snprintf(tmp_path, PATH_MAX, "/tmp/%s-TagFS-QREAD", base);
        fd = open(tmp_path, O_RDONLY);
        f_info->fh = fd;
        g_free(basec);
        basec = NULL;
        return retstat;
    }
    g_free(basec);
    basec = NULL;

    char *fpath = get_id_copies_path(path);
    fd = open(fpath, f_info->flags);

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
        char *cmdstr = g_strstrip(g_strdup(buf));
        result_t *res = tagdb_query(TAGFS_DATA->db, cmdstr);
        if (res != NULL)
        {
            log_msg("tagfs_write #LISTEN# tagdb_query result type=%d\
                    value=%s\n", res->type, tagdb_value_to_str(res->type, &(res->data)));
            char *qrstr = g_strdup_printf("#QREAD-%d#", (int) fuse_get_context()->pid);
            if (!result_queue_exists(TAGFS_DATA->rqm, qrstr))
                result_queue_new(TAGFS_DATA->rqm, qrstr);
            result_queue_add(TAGFS_DATA->rqm, qrstr, res);
            log_msg("tagfs_write rqm=%p\n", TAGFS_DATA->rqm);
            g_free(qrstr);
        }
        g_free(cmdstr);
        //result_destroy(&res);
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
        char *result_string = tagdb_value_to_str(res->type, &(res->data));
        int reslen = strlen(result_string);
        memcpy(buffer, result_string, (reslen<=size)?reslen:size);
        result_destroy(&res);
        return (reslen<=size)?reslen:size;
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
            tagfs_error("tagfs_truncate truncate");
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
    int fd;
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

// remove the file for real and remove from the
// db structure
int tagfs_unlink (const char *path)
{
    int retstat = 0;
    char *fpath = NULL;
    // get the file's id
    // since there isn't a unique "name" for a file
    // we have to get the intersection of the files
    // from our current query and the "name" tag.
    // if there's more than one file with that tag
    // (why would you do that??) remove the one with
    // the highest id.
    char *qstring = path_to_qstring(path, TRUE);
    result_t *res = tagdb_query(TAGFS_DATA->db, qstring);
    if (res->type == -1)
        return -1; // might do something more sophisticated later

    if (res->type == tagdb_dict_t)
    {
        int max = 0;
        int maxlen = 16; // maximum length of an id
        GHashTableIter it;
        gpointer k, v;

        g_hash_loop(res->data.d, it, k, v)
        {
            if (max < GPOINTER_TO_INT(k))
            {
                max = GPOINTER_TO_INT(k);
            }
        }
        g_free(qstring);
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
        union tagdb_value *freeme = NULL;
        g_hash_loop(res->data.d, it, k, v)
        {
            // convert the id to a string if we don't have a name and use that
            int namecode = tagdb_get_tag_code(TAGFS_DATA->db, "name");
            union tagdb_value *name = tagdb_get_sub(TAGFS_DATA->db, GPOINTER_TO_INT(k), namecode, FILE_TABLE);
            if (name == NULL)
            {
                freeme = tagdb_str_to_value(tagdb_str_t, "!NO_NAME!");
                name = freeme;
            }
            log_msg("calling filler with name %s\n", name->s);
            if (filler(buffer, name->s, NULL, 0) != 0)
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
        // we take the union of tags from the files too
        GList *all_tags = g_hash_table_get_values(res->data.d);
        GHashTable *tag_union = set_union(all_tags);
        g_hash_loop(tag_union, it, k, v)
        {
            char *tagname = tagdb_get_tag_value(TAGFS_DATA->db, TO_I(k));
            if (filler(buffer, tagname, NULL, 0) != 0)
            {
                // might need something to free
                // the result_t...
                //g_hash_table_unref(res->data.d);
                g_free(res);
                log_msg("    ERROR tagfs_readdir filler:  buffer full");
                return -errno;
            }
            dircount++;
        }
        if (dircount == 0)
        {
            g_free(res);
            return -ENOENT;
        }
    }
    //g_free(res->data.b);
    g_free(res);
    return 0;
}

int tagfs_rmdir (const char *path)
{
    char *basec = NULL;
    char *base = NULL;
    basec = g_strdup(path);
    base = basename(basec);

    char *qstring = g_strdup_printf("TAG REMOVE %s", base);
    tagdb_query(TAGFS_DATA->db, qstring);
    return 0;
}

void tagfs_destroy (void *user_data)
{
    tagdb_save(TAGFS_DATA->db, NULL, NULL);
    fclose(TAGFS_DATA->logfile);
}
// mkdir won't be included because you can't add type 
// to tag on creation and the filesystem will report
// failure because it can't stat an empty tag
// 
// rmdir on the other hand makes sense
struct fuse_operations tagfs_oper = {
    .mknod = tagfs_mknod,
    //.mkdir = tagfs_mkdir,
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

int main (int argc, char **argv)
{
    int i;
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
        fprintf(stderr, "Running TAGFS as root opens unnacceptable security holes\n");
        return 1;
    }
    tagfs_data = calloc(sizeof(struct tagfs_state), 1);
    if (tagfs_data == NULL)
    {
        perror("Cannot alloc tagfs_data");
        abort();
    }
    tagfs_data->logfile = log_open("tagfs.log");
    tagfs_data->debug = FALSE;
    tagfs_data->db = newdb("test.db", "test.types");

    for (i = 1; (i < argc) && (argv[i][0] == '-'); i++)
        if (argv[i][1] == 'o') i++; // -o takes a parameter; need to
    // skip it too.  This doesn't
    // handle "squashed" parameters
    if ((argc - i) != 2) abort();
    char copiespath[PATH_MAX];
    char mountpath[PATH_MAX];
    tagfs_data->copiesdir = realpath(argv[i], copiespath);
    tagfs_data->mountdir = realpath(argv[i+1], mountpath);
    printf("tagfs_data->copiesdir = \"%s\"\n", tagfs_data->copiesdir);
    printf("tagfs_data->mountdir = \"%s\"\n", tagfs_data->mountdir);
    if (tagfs_data->copiesdir == NULL || tagfs_data->mountdir == NULL) abort();
    tagfs_data->listen = "#LISTEN#";
    tagfs_data->rqm = malloc(sizeof(ResultQueueManager));
    tagfs_data->rqm->queue_table = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify) g_free, 
            (GDestroyNotify) g_queue_free);
    argv[i] = argv[i+1];
    argc--;

    fprintf(stderr, "about to call fuse_main\n");
    fuse_stat = fuse_main(argc, argv, &tagfs_oper, tagfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    return fuse_stat;
}
