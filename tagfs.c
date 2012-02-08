/* must be before fuse.h */
#include "tagfs.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>

#include "tagdb.h"

struct tagfs_state {
    char *copiesdir;
    char *mountdir;
    tagdb *db;
};
#define TAGFS_DATA ((struct tagfs_state *) fuse_get_context()->private_data)

// returns the file in our copies directory corresponding to
// the one in path
// should only be called on regular files since
// directories are only virtual
static char *tagfs_realpath(const char *path)
{
    return g_strconcat(TAGFS_DATA->copiesdir, path, NULL);
}

// all dirs have the same permissions as the
// copies dir.
// a file is a dir if i can't find a file that matches
// that path
int tagfs_getattr (const char *path, struct stat *statbuf)
{
    FILE *log = fopen("tagfs.log", "a");
    path = tagfs_realpath(path);
    fprintf(log, "path: %s\n", path);
    fclose(log);
    if (tagdb_path_to_node(
    
    return lstat(path, statbuf);
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

int tagfs_mknod (const char *path, mode_t mode, dev_t dev)
{
    /*
    char *base_copy;
    char *base;
    base_copy = strdup(path);
    base = basename(base_copy);
    create_tag(base);
    */
    return 0;
}

int tagfs_mkdir (const char *path, mode_t mode)
{
    create_tag(path);
    return 0;
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
    GNode *cur_node = tagdb_path_to_node(tagdb_toTagTree(TAGFS_DATA->db),
            path);
    if (cur_node == NULL)
    {
        return -1;
    }
    else
    {
        cur_node == g_node_first_child(cur_node);
        while (cur_node != NULL)
        {
            filler(buffer, cur_node->data, NULL, 0);
            cur_node = g_node_next_sibling(cur_node);
        }
    }
    return 0;
}

// Add a tag to a file
/*
 * Looks at the file inode and adds a tag to our database.
 * Called when a file is created in a tag folder or moved 
 * between folders
 */
int tag_file (char *path, const char *tag)
{
    insert_file_tag(TAGFS_DATA->db, basename(path), tag);
    return 0;
}


struct fuse_operations tagfs_oper = {
    .mknod = tagfs_mknod,
    .mkdir = tagfs_mkdir,
    .release = tagfs_release,
//    .readdir = tagfs_readdir,
//    .getattr = tagfs_getattr,
};

int main (int argc, char **argv)
{
    /*
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
    argv[i] = argv[i+1];
    argc--;

    fprintf(stderr, "about to call fuse_main\n");
    fuse_stat = fuse_main(argc, argv, &tagfs_oper, tagfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    return fuse_stat;
    */
}
