#include "util.h"
#include "tagdb.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

typedef int (*fuse_fill_dir_t) (void *buf, char *name,
        const struct stat *stbuf, off_t off);
static char *test_realpath(const char *path)
{
    return g_strconcat("copies", "/", path, NULL);
}

// buffer is a 2d char array
int fill_dir(char **b, char *en, const struct stat *sb,
        off_t ofs)
{
    //ignore ofs
    int i = 0;
    while (i < 20 && b[i] != NULL)
    {
        i++;
    }
    b[i] = g_strdup(en);
    //ignore statbuf
    return 0;
}

int test_getattr (tagdb *db, const char *path, struct stat *statbuf)
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

    // check if the file is a tag
    GList *dir = g_list_find_custom(get_tag_list(db), 
                base, (GCompareFunc) g_strcmp0);
    if (dir != NULL) 
    {
        statbuf->st_mode = S_IFDIR | 0755;
    }
    
    // is our file in the database?
    gpointer file = tagdb_get(db, base);
    if (file != NULL)
    {
        fpath = test_realpath(base);
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

int test_readdir (tagdb *db, const char *path, void *buffer, fuse_fill_dir_t filler,
        off_t offset)
{
    struct stat statbuf;
    memset(&statbuf, 0, sizeof(statbuf));
    statbuf.st_mode = DT_DIR << 12;
    filler(buffer, ".", &statbuf, 0);
    filler(buffer, "..", &statbuf, 0);


    char *itemscopy = strdup(path);
    GList *items = pathToList(itemscopy);
    free(itemscopy);

    GList *files = get_files_by_tag_list(db, items);
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

int main (int argc, char **argv)
{
    char **buffer;
    int i, j;
    tagdb *thedb = newdb("test.db", "tags.list");
    buffer = calloc(20, sizeof(char*));
    for (i = 0; i < 20; i++)
    {
        buffer[i] = NULL;
    }
    struct stat statbuf;
    for (i = 0; i < 6000; i++)
    {
        test_readdir(thedb, "/file", buffer, fill_dir, 0);
        memset(&statbuf, 0, sizeof(statbuf));
        test_getattr(thedb, "/file", &statbuf);
    }
    /*
    for (i = 0; i < 20 && strcmp(buffer[i], "") != 0; i++)
    {
        printf("%s\n", buffer[i]);
    }
    */
    return 0;
}
