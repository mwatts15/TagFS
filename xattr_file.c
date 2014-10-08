/* This is a wrapper for the File object that
 * manipulates the backing file associated to
 * it (if there is one) in order to provide
 * persistence and portability to the file
 * system.
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <attr/libattr.h>
#include <attr/xattr.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "path_util.h"

#define XATTR_DOMAIN "user.tagfs."

void xattr_file_add_tag (char *path, Tag *t, tagdb_value_t *v)
{
    int fd = open(path, 0);
    const char *name = tag_name(t);
    char *xattr_name = g_strdup_printf(XATTR_DOMAIN "%s", name);
    char *xattr_value = tagdb_value_to_str(v);
    fsetxattr(fd, xattr_name, xattr_value, strlen(xattr_value), 0);

    g_free(xattr_value);
    g_free(xattr_name);
}
