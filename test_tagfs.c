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
#include <libgen.h>
#include <unistd.h>
#include <sys/types.h>

typedef int (*fuse_fill_dir_t) (char **buf, char *name,
        const struct stat *stbuf, off_t off);

struct tagfs_state {
    char *copiesdir;
    char *mountdir;
    char *listen;
    tagdb *db;
    FILE *logfile;
} TAGFS_DATA;

static char *test_realpath(const char *path)
{
    return g_strconcat("copies", "/", path, NULL);
}

char *path_to_qstring (const char *path, gboolean is_file_path)
{
    char *dir;
    char *base;
    char *dircopy;
    char *basecopy;
    char *qstring;

    dircopy = g_strdup(path);
    dir = dirname(dircopy);
    qstring = malloc(sizeof(char) * strlen(path) + 24);
    if (is_file_path)
    {
        basecopy = g_strdup(path);
        base = basename(basecopy);
        sprintf(qstring, "TAG TSPEC %s/name=%s", dir, base);
    }
    else
    {
        sprintf(qstring, "TAG TSPEC %s", dir);
    }
    g_free(basecopy);
    g_free(dircopy);
    return qstring;
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

int test_getattr (const char *path, struct stat *statbuf)
{
    int retstat = 0;
    char *fpath;
    char *basecopy; 
    char *base; 
    char *qstring;
    result_t *res;

    memset(statbuf, 0, sizeof(statbuf));
    /*
       fprintf(log, "calling getattr on\n");
       fprintf(log, "path=%s\n", path);
     */
    //fprintf(TAGFS_DATA.logfile, "tagfs_getattr (path=%s, statbuf=%p)\n", path, statbuf);
    if (g_strcmp0(path, "/") == 0)
    {
        statbuf->st_mode = S_IFDIR | 0755;
        return retstat;
    }

    basecopy = g_strdup(path);
    base = basename(basecopy);

    if (g_strcmp0(base, TAGFS_DATA.listen) == 0)
    {
        statbuf->st_mode = S_IFREG | 0755;
        g_free(basecopy);
        return retstat;
    }

    // check if the file is a tag
    if (tagdb_get_tag_code(TAGFS_DATA.db, base) > 0)
    {
        statbuf->st_mode = S_IFDIR | 0755;
        g_free(basecopy);
        return retstat;
    }

    qstring = path_to_qstring(path, TRUE);
    res = tagdb_query(TAGFS_DATA.db, qstring);
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
            fpath = test_realpath(id_string);
            retstat = lstat(fpath, statbuf);
            g_free(fpath);
            return retstat;
        }
    }
        return -ENOENT;
}

int test_readdir (const char *path, void *buffer, fuse_fill_dir_t filler,
        off_t offset)
{
    struct stat statbuf;
    memset(&statbuf, 0, sizeof(statbuf));
    statbuf.st_mode = DT_DIR << 12;
    filler(buffer, ".", &statbuf, 0);
    filler(buffer, "..", &statbuf, 0);

    char *qstring = path_to_qstring(path, FALSE);
    result_t *res = tagdb_query(TAGFS_DATA.db, qstring);

    if (res->type == tagdb_dict_t)
    {
        GHashTableIter it;
        gpointer k, v;
        int dircount = 0;
        //fprintf(TAGFS_DATA.logfile, "tagfs_readdir (path=%s, buffer=%p, filler=%p, etc.)\n", path, buffer, filler);
        g_hash_loop(res->data.d, it, k, v)
        {
            // convert the id to a string if we don't have a name and use that
            if (filler(buffer, (gchar*) v, NULL, 0) != 0)
            {
                // might need something to free
                // the result_t...
                //g_hash_table_unref(res->data.d);
                g_free(res);
                return -errno;
            }
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

int main (int argc, char **argv)
{
    char **buffer;
    int i, j;
    TAGFS_DATA.db = newdb("test.db", "test.types");
    buffer = calloc(20, sizeof(char*));
    for (i = 0; i < 20; i++)
    {
        buffer[i] = NULL;
    }
    struct stat statbuf;
    for (i = 0; i < 2000; i++)
    {
        test_getattr("/", &statbuf);
        test_getattr("/file001", &statbuf);
        test_readdir("/", buffer, fill_dir, 0);
    }
    return 0;
}
