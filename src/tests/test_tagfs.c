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

#include "util.h"
#include "tagdb.h"

typedef int (*fuse_fill_dir_t) (char **buf, char *name,
        const struct stat *stbuf, off_t off);

struct tagfs_state {
    char *copiesdir = NULL;
    char *mountdir = NULL;
    char *listen = NULL;
    tagdb *db = NULL;
    FILE *logfile = NULL;
};
struct tagfs_state *TAGFS_DATA = NULL;
FILE *log_open (const char *name)
{
    FILE *logfile = NULL;

    // very first thing, open up the logfile and mark that we got in
    // here.  If we can't open the logfile, we're dead.
    logfile = fopen(name, "w");
    if (logfile == NULL) {
        perror("logfile");
        exit(EXIT_FAILURE);
    }

    // set logfile to line buffering
    setvbuf(logfile, NULL, _IOLBF, 0);

    return logfile;
}

void log_msg(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
}

void log_data ()
{
    log_msg("TAGFS_DATA->copiesdir = %s\n", TAGFS_DATA->copiesdir);
    log_msg("TAGFS_DATA->mountdir = %s\n", TAGFS_DATA->mountdir);
    log_msg("TAGFS_DATA->listen = %s\n", TAGFS_DATA->listen);
    log_msg("TAGFS_DATA->db = %p\n", TAGFS_DATA->db);
    log_msg("TAGFS_DATA->db->db_fname = %s\n", TAGFS_DATA->db->db_fname);
    log_msg("TAGFS_DATA->db->types_fname = %s\n", TAGFS_DATA->db->types_fname);
    log_msg("TAGFS_DATA->db->tables[0] = %p\n", TAGFS_DATA->db->tables[0]);
    print_hash(TAGFS_DATA->db->tables[0]);
    log_msg("TAGFS_DATA->db->tables[1] = %p\n", TAGFS_DATA->db->tables[1]);
    print_hash(TAGFS_DATA->db->tables[1]);
}

static char *tagfs_realpath(const char *path)
{
    return g_strconcat("copies", "/", path, NULL);
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

// buffer is a 2d char array
int fill_dir (char **b, char *en, const struct stat *sb,
        off_t ofs)
{
    //ignore ofs
    int i = 0;
    while (i < 50 && b[i] != NULL)
    {
        i++;
    }
    if (i == 50)
        return 1;

    b[i] = g_strdup(en);
    //ignore statbuf
    return 0;
}

void clr_buffer (char **buf, int size)
{
    int i;
    for (i = 0; i < size; i++)
        buf[i] = NULL;
}

int test_getattr (const char *path, struct stat *statbuf)
{
    int retstat = 0;
    char *fpath = NULL;
    char *basecopy; 
    char *base; 
    char *qstring = NULL;
    result_t *res = NULL;

    log_msg("\nbb_getattr(path=\"%s\", statbuf=0x%08x)\n",
	  path, statbuf);

    memset(statbuf, 0, sizeof(statbuf));
    if (g_strcmp0(path, "/") == 0)
    {
        lstat(TAGFS_DATA->mountdir, statbuf);
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
int test_readdir (const char *path, void *buffer, fuse_fill_dir_t filler,
        off_t offset, void *f_info)
{
    struct stat statbuf;
    lstat(TAGFS_DATA->mountdir, &statbuf);
    filler(buffer, ".", &statbuf, 0);
    filler(buffer, "..", &statbuf, 0);

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

int main (int argc, char **argv)
{
    char **buffer = NULL;
    int i;
    TAGFS_DATA = malloc(sizeof(struct tagfs_state));
    TAGFS_DATA->db = newdb("test.db", "test.types");
    TAGFS_DATA->logfile = log_open("test.log");
    TAGFS_DATA->copiesdir = realpath(argv[0], NULL);
    TAGFS_DATA->mountdir = realpath(argv[1], NULL);
    TAGFS_DATA->listen = "#LISTEN#";
    log_data();
    buffer = calloc(50, sizeof(char*));
    for (i = 0; i < 50; i++)
    {
        buffer[i] = NULL;
    }
    struct stat statbuf;
    for (i = 0; i < 3; i++)
    {
        test_getattr("/file001", &statbuf);
        test_getattr("/", &statbuf);
        test_readdir("/\\name", buffer, fill_dir, 0, NULL);
        test_readdir("/name~tag048", buffer, fill_dir, 0, NULL);
        clr_buffer(buffer, 50);
    }
    return 0;
}
