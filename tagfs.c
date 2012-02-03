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

// Gives the path to the file in our special directory
static void tagfs_realpath(char fpath[PATH_MAX], const char *path)
{
    strcpy(fpath, TAGFS_DATA->mountdir);
    strncat(fpath, path, PATH_MAX);
}

int tagfs_getattr (const char *path, struct stat *statbuf)
{
    char real_path[PATH_MAX];
    tagfs_realpath(real_path, path);
    lstat(real_path, statbuf);
}

int tagfs_mknod (const char *path, mode_t mode, dev_t dev)
{
    char *base_copy;
    char *base;
    base_copy = strdup(path);
    base = basename(base_copy);
    create_tag(base);
}

int tagfs_mkdir (const const *path, mode_t mode)
{
    create_tag(path);
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
    GNode *cur_node = path_to_node(tagdb_toTagTree(TAGFS_DATA->db),
            path);
    if (cur_node == NULL)
    {
        return 0;
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
}

// Create a tag
void create_tag (const char *tag)
{
    insert_tag(TAGFS_DATA->db, tag);
}

// Add a tag to a file
/*
 * Looks at the file inode and adds a tag to our database.
 * Called when a file is created in a tag folder or moved 
 * between folders
 */
int tag_file (const char *path, const char *tag)
{
    insert_file_tag(TAGFS_DATA->db, basename(path), tag);
}


struct fuse_operations tagfs_oper = {
    .mknod = tagfs_mknod,
    .mkdir = tagfs_mkdir,
    .release = tagfs_release
};

int main (int argc, char **argv)
{
    create_tag("long");
    create_tag("height");
    return 0;
}
