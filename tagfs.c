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

struct tagfs_state {
    char *copiesdir;
    char *mountdir;
    char *tagfile;
    char **tags;
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

// Create a tag
void create_tag (const char *tag)
{
    FILE *file = fopen("tagdb", "r+");
    int tag_length = strlen(tag);
    char line[tag_length + 1];

    rewind(file);
    int ret = fscanf(file, "%s", line);
    while (fgets(line, tag_length + 1, file) != NULL)
    {
        // We might have read a whole line or we might not have
        // First we check if the string we got matches our string
        if (strcmp(tag, line) == 0)
        {
            // If our string is already in the file, then we're good
            // we can just move on.
            return;
        }
        ret = fscanf(file, "%s", line);
    }
    fprintf(file, "%s\n", tag); 
}

// Add a tag to a file
/*
 * Looks at the file inode and adds a tag to our database.
 * Called when a file is created in a tag folder or moved 
 * between folders
 */
int tag_file (const char *path)
{
}
/*
struct fuse_operations tagfs_oper = {
    .mknod = tagfs_mknod,
    .release = tagfs_release
};
*/
int main (int argc, char **argv)
{
    create_tag("long");
    create_tag("height");
    return 0;
}
