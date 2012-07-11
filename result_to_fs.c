/* Representations of basic types in the filesystem.

   Dictionaries
   - Dictionaries are given as directories. The keys, whether
     Integer or String type will be given as the names of
     regular files. If the type of the value for a file is a
     container type (List or Dictionary) then the file presented
     will also be a directory with contents structured as
     described herein. Otherwise, the values are presented in
     ToString format as the content of the files. The sizes of
     files will be either the string length of the binstring
     length in the case of a binstring type.

   Lists
   - Lists are given as directories. The values are presented as
     file contents with the file names being integers counting
     off the files. The type (regular file or directory) is
     determined the same as for dictionaries.

   Integers
   - Integers are given as regular files, the contents being a
     a string representation of the integer.

   Strings and Errors
   - Strings are given as regular files, the contents being the
     string. Errors have a string representation.

   Binary
   - Binary data are given as regular files, the contents being
     the ToString representation of the data
*/

/* Each of these takes a "directory filler", a buffer to pass
   to it, and a container type result_t*. The names are given as
   described at the top of this document. */
#include <string.h>
#include "log.h"
#include "result_to_fs.h"

static int _log_level = 0;
void tagdb_value_fs_readdir (result_t *r, void *buffer, fuse_fill_dir_t filler)
{
    switch (r->type)
    {
        case tagdb_list_t:
            {
                size_t i = 0;
                LL(r->data.l, it)
                {
                    char *name = g_strdup_printf("%zd", i);
                    filler(buffer, name, NULL, 0);
                    g_free(name);
                    i++;
                    LL_END;
                }
            }
            break;
        case tagdb_dict_t:
            HL(r->data.d, it, k, v)
            {
                char *name = tagdb_value_to_str(k);
                log_msg("tagdb_value_fs_readdir filling with %s\n", name);
                filler(buffer, name, NULL, 0);
                g_free(name);
                HL_END;
            }
            break;
        default:
            break;
    }
}

/* Each of these takes a stat* and a result_t* and fills in the
   statbuf with the size and type as described at the top of this
   document */
void tagdb_value_fs_stat (result_t *r, struct stat *statbuf)
{
    off_t size = 0;
    switch (r->type)
    {
        case tagdb_list_t:
        case tagdb_dict_t:
            statbuf->st_mode = S_IFDIR | 0755;
            return;
            break;
        case tagdb_int_t:
            {
                size_t n = r->data.i;
                if (n == 0)
                    size = 1;
                else
                    for (; n != 0; n = n / 10) size++;
                log_msg("our_size = %zd\n");
            }
            break;
        case tagdb_str_t:
        case tagdb_err_t:
            size = strlen(r->data.s);
            break;
        case tagdb_bin_t:
            size = r->data.b->size;
            break;
    }
    statbuf->st_mode = S_IFREG | 0444;
    statbuf->st_size = size;
}

/* Takes an offset, a size, a buffer and a result_t*
   and returns the appropriate part of the string representation of
   the result_t */
size_t tagdb_value_fs_read (result_t *r, char *buf, size_t size, off_t offset)
{
    size_t res = 0;
    size_t len = 0;
    char *str = NULL;
    char *freeme = NULL;
    switch (r->type)
    {
        /* You can't read from the middle of an int file,
           so the offset is ignored */
        case tagdb_int_t:
            // convert to a string
            str = tagdb_value_to_str(r);
            len = strlen(str);
            freeme = str;
            break;
        case tagdb_str_t:
        case tagdb_err_t:
            len = strlen(r->data.s);
            str = r->data.s;
            break;
        case tagdb_bin_t:
            len = r->data.b->size;
            // offsets past the type string and size
            str = r->data.b->data + 1 + sizeof(size_t);
            break;
        default:
            break;
    }
    if (offset < len)
    {
        str = str + offset;
        len = len - offset;
        size_t real_size = (len<=size)?len:size;
        memcpy(buf, str, real_size);
        res = real_size;
    }
    g_free(freeme);
    return res;
}

/* Reads off the first element in a path
   and puts the rest of the path in it's
   second arg */
#define chug_path(path, first_path, rest_path) \
char first_path[strlen(path)]; \
char *rest_path; \
{ \
    char *after_root = g_path_skip_root(path); \
    size_t part_length = strcspn(after_root, "/"); \
    g_memmove(first_path, after_root, part_length); \
    first_path[part_length] = 0; \
    rest_path = after_root + part_length; \
} \


/* translates a path into a result. The path is
   assumed to be "underneath" a container result type.
   Dictionaries will see each path element as a key into itself
   and will return the value associated. Lists take each path
   component to be an index into the list */
result_t *tagdb_value_fs_path_to_result (result_t *r, const char *path)
{
    if (!r)
        return r;
    if (path == NULL || strlen(path) == 0)
        return r;
    chug_path(path, first, rest);
    log_msg("path_to_result first=%s rest=%s\n", first, rest);
    switch (r->type)
    {
        case tagdb_int_t:
        case tagdb_str_t:
        case tagdb_bin_t:
        case tagdb_err_t:
            return r;
        case tagdb_list_t:
            {
                gint64 idx = strtoll(first, NULL, 10);
                GList *l = g_list_nth(r->data.l, idx);
                if (l)
                {
                    return tagdb_value_fs_path_to_result(l->data, rest);
                }
                return NULL;
            }
        case tagdb_dict_t:
            {
                result_t *res = tagdb_value_dict_lookup_data(r, first);
                return tagdb_value_fs_path_to_result(res, rest);
            }
        default:
            return NULL;
    }
}
