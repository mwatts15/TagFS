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

typedef int (*fuse_fill_dir_t) (char **buf, char *name,
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
    if (g_strcmp0(path, "/") == 0)
    {
        statbuf->st_mode = S_IFDIR | 0755;
        g_free(basecopy);
        return retstat;
    }

    if (g_strcmp0(base, "listen") == 0)
    {
        statbuf->st_mode = S_IFREG | 0755;
        g_free(basecopy);
        return retstat;
    }

    // check if the file is a tag
    GHashTable *dir = tagdb_get_tag_files(db, base);
    if (dir != NULL) 
    {
        statbuf->st_mode = S_IFDIR | 0755;
        g_free(basecopy);
        return retstat;
    }
    
    // is our file in the database?
    GHashTable *file = tagdb_get_file_tags(db, base);
    if (file != NULL)
    {
        fpath = test_realpath(base);
        retstat = lstat(fpath, statbuf);
        g_free(fpath);
        g_free(basecopy);
        return retstat;
    }
    else
    {
        return -ENOENT;
    }
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
    print_string_list(items);
    free(itemscopy);

    GList *files = get_files_by_tag_list(db, items);
    int dircount = 0;
    GList *it = files;
    print_list(stderr, files);
    while (it != NULL)
    {
        char *fname = code_table_get_value(db->file_codes, GPOINTER_TO_INT(it->data));
        printf("fname : %s\n", fname);
        if (filler(buffer, fname, NULL, 0) != 0)
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
    tagdb *thedb = newdb("test.db");
    buffer = calloc(20, sizeof(char*));
    for (i = 0; i < 20; i++)
    {
        buffer[i] = NULL;
    }
    struct stat statbuf;
    for (i = 0; i < 1; i++)
    {
        test_readdir(thedb, "/tag026/tag039", buffer, fill_dir, 0);
        memset(&statbuf, 0, sizeof(statbuf));
        test_getattr(thedb, "/file", &statbuf);
        test_getattr(thedb, "/", &statbuf);
        test_getattr(thedb, "/listen", &statbuf);
        test_getattr(thedb, "/NOTINTHERE", &statbuf);
    }
    return 0;
}
