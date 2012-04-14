/* must be before fuse.h */
#include "tagfs.h"
#include "util.h"
#include "cmd.h"
#include "params.h"
#include "types.h"
#include "log.h"

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

#include "tagdb.h"

// returns the file in our copies directory corresponding to
// the one in path
// should only be called on regular files since
// directories are only virtual
static char *tagfs_realpath(const char *path)
{
    char *res = g_strconcat(TAGFS_DATA->copiesdir, "/", path, NULL);
    return res;
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

// all dirs have the same permissions as the
// mount dir.
// a file is a dir if i can't find a file that matches
// that path
int tagfs_getattr (const char *path, struct stat *statbuf)
{
    int retstat = 0;
    char *fpath;
    char *basecopy; 
    char *base; 
    char *qstring;
    result_t *res;

    log_msg("\nbb_getattr(path=\"%s\", statbuf=0x%08x)\n",
	  path, statbuf);

    memset(statbuf, 0, sizeof(statbuf));
    if (g_strcmp0(path, "/") == 0)
    {
        //lstat(TAGFS_DATA->mountdir, statbuf);
        statbuf->st_mode = S_IFDIR | 0755;
        return retstat;
    }

    basecopy = g_strdup(path);
    base = basename(basecopy);

    if (g_strcmp0(base, TAGFS_DATA->listen) == 0)
    {
        statbuf->st_mode = S_IFREG | 0755;
        g_free(basecopy);
        return retstat;
    }

    // check if the file is a tag
    if (tagdb_get_tag_code(TAGFS_DATA->db, base) > 0)
    {
        statbuf->st_mode = S_IFDIR | 0755;
        g_free(basecopy);
        return retstat;
    }
    // well, does a query return anything?
    // if so we'll take it
    qstring = path_to_qstring(path, FALSE);
    res = tagdb_query(TAGFS_DATA->db, qstring);
    g_free(qstring);
    if (res->type == tagdb_dict_t && g_hash_table_size(res->data.d) > 0)
    {
        statbuf->st_mode = S_IFDIR | 0755;
        g_free(basecopy);
        g_free(res);
        g_free(qstring);
        return retstat;
    }
    g_free(res);
    // it's got to be a file, right?
    qstring = path_to_qstring(path, TRUE);
    res = tagdb_query(TAGFS_DATA->db, qstring);
    g_free(qstring);
    if (res->type == tagdb_dict_t)
    {
        gpointer k, v;
        GHashTableIter it;
        int maxlen = 16;
        char id_string[maxlen];
        g_hash_loop(res->data.d, it, k, v)
        {
            int length = g_snprintf(id_string, maxlen, "%d", GPOINTER_TO_INT(k));
            if (length >= maxlen)
            {
                fprintf(stderr, "id too large in getattr\n");
                exit(-1);
            }
            //int nc  = tagdb_get_tag_code(TAGFS_DATA->db, "name");
            //log_msg("getattr, real_filename=%s\n", g_hash_table_lookup(v, GINT_TO_POINTER(nc)));
            fpath = tagfs_realpath(id_string);
            retstat = lstat(fpath, statbuf);
            g_free(fpath);
            g_free(res);
            return retstat;
        }
    }
    // oh noes
    g_free(res);
    return -ENOENT;
}

int tagfs_create (const char *path, mode_t mode, struct fuse_file_info *fi)
{
    char *base;
    char *basecopy;
    char *fpath;
    result_t *res = NULL;
    char *qstring = NULL;
    int fd;
    int retstat;
    int maxlen = 16;
    char id_string[maxlen];
    basecopy = g_strdup(path);
    base = basename(basecopy);

    log_msg("\nbb_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n",
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
    result_t *res;
    int fd;
    char *fpath;
    char *qstring;
    log_msg("\nbb_open(path\"%s\", f_info=0x%08x)\n",
	    path, f_info);
    
    qstring = path_to_qstring(path, TRUE);
    res = tagdb_query(TAGFS_DATA->db, qstring);
    g_free(qstring);
    if (res->type == tagdb_dict_t)
    {
        gpointer k, v;
        GHashTableIter it;
        int maxlen = 16;
        char id_string[maxlen];
        g_hash_loop(res->data.d, it, k, v)
        {
            int length = g_snprintf(id_string, maxlen, "%d",
                    GPOINTER_TO_INT(k));
            if (length >= maxlen)
            {
                fprintf(stderr, "id too large in open\n");
                exit(-1);
            }
            fpath = tagfs_realpath(id_string);
            fd = open(fpath, f_info->flags);

            f_info->fh = fd;
    
            g_free(fpath);
        }
    }
    return retstat;
}

int tagfs_write (const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    char *basecopy = g_strdup(path);
    char *base = basename(basecopy);

    // check if we're writing to the "listen" file
    if (g_strcmp0(base, TAGFS_DATA->listen) == 0)
    {
        // sanitize the buffer string
        char *cmdstr = g_strstrip(g_strdup(buf));
        tagdb_query(TAGFS_DATA->db, cmdstr);
        g_free(basecopy);
        g_free(cmdstr);
        return 0;
    }
    g_free(basecopy);
    return pwrite(fi->fh, buf, size, offset);
}

int tagfs_read (const char *path, char *buffer, size_t size, off_t offset,
        struct fuse_file_info *f_info)
{
    log_msg("\nbb_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, f_info=0x%08x)\n",
	    path, buffer, size, offset, f_info);
    // no need to get fpath on this one, since I work from f_info->fh not the path
    log_fi(f_info);

    int retstat = 0;
    retstat = pread(f_info->fh, buffer, size, offset);
    return retstat;
}

// no clue why this is needed, but whatever
int tagfs_flush(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    return retstat;
}

int tagfs_truncate(const char *path, off_t newsize)
{
    int retstat = 0;
    char *fpath;
    char *base;
    char *basecopy;
    basecopy = g_strdup(path);
    base = basename(basecopy);
    
    fpath = tagfs_realpath(base);
    
    retstat = truncate(fpath, newsize);
    
    g_free(fpath);
    g_free(basecopy);
    return retstat;
}

int tagfs_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    
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
    char *base;
    char *basecopy;
    char *fpath;
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
    char *fpath;
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
    /*
    struct stat statbuf;
    int stat = lstat(TAGFS_DATA->mountdir, &statbuf);
    log_msg("lstat returns %d for %s\n", stat, TAGFS_DATA->mountdir);
    log_msg("__HERE__\n");
    */
    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);

    char *qstring = path_to_qstring(path, FALSE);
    log_msg("\nbb_readdir(path=\"%s\", buffer=0x%08x, filler=0x%08x, offset=%lld, f_info=0x%08x)\n",
	    path, buffer, filler, offset, f_info);
    result_t *res = tagdb_query(TAGFS_DATA->db, qstring);
    g_free(qstring);

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
                log_msg("    ERROR bb_readdir filler:  buffer full");
                return -errno;
            }
            g_free(freeme);
            dircount++;
        }
        if (dircount == 0)
        {
            return -ENOENT;
        }
    }
    //g_free(res->data.b);
    g_free(res);
    return 0;
}

void tagfs_destroy (void *user_data)
{
    tagdb_save(TAGFS_DATA->db, NULL, "test.types");
}

struct fuse_operations tagfs_oper = {
    .mknod = tagfs_mknod,
    //.mkdir = tagfs_mkdir,
    .release = tagfs_release,
    //.opendir = tagfs_opendir,
    //.releasedir = tagfs_releasedir, 
    .readdir = tagfs_readdir,
    .getattr = tagfs_getattr,
    .unlink = tagfs_unlink,
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
    struct tagfs_state *tagfs_data;

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
    printf("tagfs_data->mountdir = \"%s\"\n", tagfs_data->mountdir);
    if (tagfs_data->copiesdir == NULL || tagfs_data->mountdir == NULL) abort();
    tagfs_data->listen = "#LISTEN#";
    argv[i] = argv[i+1];
    argc--;

    fprintf(stderr, "about to call fuse_main\n");
    fuse_stat = fuse_main(argc, argv, &tagfs_oper, tagfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    return fuse_stat;
}
