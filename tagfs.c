/* must be before fuse.h */
#include "tagfs.h"
#include "util.h"
#include "cmd.h"
#include "params.h"

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

void print_list(FILE *out, GList *l)
{
    putc('(', out);
    while (l != NULL)
    {
        fprintf(out, "%s", (char*) (l->data));
        if (g_list_next(l) != NULL)
        {
            putc(' ', out);
        }
        l = g_list_next(l);
    }
    putc(')', out);
    putc('\n', out);
}

// returns the file in our copies directory corresponding to
// the one in path
// should only be called on regular files since
// directories are only virtual
static char *tagfs_realpath(const char *path)
{
    return g_strconcat(TAGFS_DATA->copiesdir, "/", path, NULL);
}

// all dirs have the same permissions as the
// mount dir.
// a file is a dir if i can't find a file that matches
// that path
int tagfs_getattr (const char *path, struct stat *statbuf)
{
    int retstat = 0;
    char *fpath;
    char *basecopy = g_strdup(path);
    char *base = basename(basecopy);

    memset(statbuf, 0, sizeof(statbuf));
    /*
    fprintf(log, "calling getattr on\n");
    fprintf(log, "path=%s\n", path);
    */
    if (g_strcmp0(path, "/") == 0)
    {
        statbuf->st_mode = S_IFDIR | 0755;
        g_free(basecopy);
        return retstat;
    }
    if (g_strcmp0(base, TAGFS_DATA->listen) == 0)
    {
        statbuf->st_mode = S_IFREG | 0755;
        g_free(basecopy);
        return retstat;
    }

    // check if the file is a tag
    GList *dir = g_list_find_custom(get_tag_list(TAGFS_DATA->db), 
                base, (GCompareFunc) g_strcmp0);
    if (dir != NULL) 
    {
        statbuf->st_mode = S_IFDIR | 0755;
    }
    
    // is our file in the database?
    gpointer file = tagdb_get(TAGFS_DATA->db, base);
    if (file != NULL)
    {
        fpath = tagfs_realpath(base);
        retstat = lstat(fpath, statbuf);
    }
    else
    {
        // we don't have it!
        return -ENOENT;
    }
    g_free(fpath);
    return retstat;
}

int tagfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    int fd;
    char *fpath;
    char *base;
    char *basecopy;
    
    basecopy = g_strdup(path);
    base = basename(basecopy);
    tagdb_insert_file(TAGFS_DATA->db, base);
    
    fpath = tagfs_realpath(path);
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
    char *fpath;
    
    fpath = tagfs_realpath(path);
    
    fd = open(fpath, f_info->flags);
    
    f_info->fh = fd;
    
    g_free(fpath);
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
        // split the string in buf
        char **command_with_args = g_strsplit(buf, " ", -1);
        // check the format of the command: we only allow
        //   alphanumeric, _, and - in the command name
        if (!str_isalnum(command_with_args[0]))
        {
            return -1;
        }
        // don't do any argument checking
        // send to do_cmd
        char *cmd = command_with_args[0];
        char **args = command_with_args + 1;
        do_cmd(cmd, (const char**) args);
        g_strfreev(command_with_args);
        g_free(basecopy);
        return 0;
    }
    g_free(basecopy);
    return pwrite(fi->fh, buf, size, offset);
}

int tagfs_read (const char *path, char *buffer, size_t size, off_t offset,
        struct fuse_file_info *f_info)
{
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
    
    fpath = tagfs_realpath(path);
    
    retstat = truncate(fpath, newsize);
    
    g_free(fpath);
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
    insert_tag(TAGFS_DATA->db, tag);
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
    char *base;
    char *basecopy;
    char *fpath;
    int retstat;
    basecopy = g_strdup(path);
    base = basename(basecopy);

    fpath = tagfs_realpath(base);
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
    char *base;
    char *basecopy;
    basecopy = g_strdup(path);
    base = basename(basecopy); // don't free base
    tagdb_remove_file(TAGFS_DATA->db, base);

    fpath = tagfs_realpath(path);
    retstat = unlink(fpath);
    g_free(basecopy);
    g_free(fpath);
    return retstat;
}

int tagfs_release (const char *path, struct fuse_file_info *f_info)
{
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
    struct stat statbuf;
    memset(&statbuf, 0, sizeof(statbuf));
    statbuf.st_mode = DT_DIR << 12;
    filler(buffer, ".", &statbuf, 0);
    filler(buffer, "..", &statbuf, 0);


    char *itemscopy = strdup(path);
    GList *items = pathToList(itemscopy);
    free(itemscopy);

    GList *files = get_files_by_tag_list(TAGFS_DATA->db, items);
    int dircount = 0;
    GList *it = files;
    while (it != NULL)
    {
        if (filler(buffer, it->data, NULL, 0) != 0)
        {
            g_list_free_full(items, g_free);
            g_list_free(files);
            return -errno;
        }
        it = g_list_next(it);
        dircount++;
    }
    if (dircount == 0)
    {
        return -ENOENT;
    }

    g_list_free_full(items, g_free);
    g_list_free(files);
    return 0;
}

// Add a tag to a file
/*
 * Looks at the file inode and adds a tag to our database.
 * Called when a file is created in a tag folder or moved 
 * between folders
 */
int tag_file (char *path,  char *tag)
{
    tagdb_insert_file_tag(TAGFS_DATA->db, basename(path), tag);
    return 0;
}

struct fuse_operations tagfs_oper = {
    .mknod = tagfs_mknod,
    //.mkdir = tagfs_mkdir, /* we don't make directories. */
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
    tagfs_data->db = newdb("test.db", "tags.list");

    for (i = 1; (i < argc) && (argv[i][0] == '-'); i++)
        if (argv[i][1] == 'o') i++; // -o takes a parameter; need to
    // skip it too.  This doesn't
    // handle "squashed" parameters
    if ((argc - i) != 2) abort();
    tagfs_data->copiesdir = realpath(argv[i], NULL);
    tagfs_data->listen = "#LISTEN#";
    argv[i] = argv[i+1];
    argc--;

    fprintf(stderr, "about to call fuse_main\n");
    fuse_stat = fuse_main(argc, argv, &tagfs_oper, tagfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    system("fusermount -u mount");
    return fuse_stat;
}
