/* Fakes some of the fuse types and functions.
 *
 * Any of the tests which use fake fuse should create their user_data
 * to fill in. It isn't necessary to pad the struct to work with everything
 */
#ifndef FAKE_FUSE_H
#define FAKE_FUSE_H

#include <unistd.h>
#include <stdint.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>

struct _fuse_context
{
    uid_t uid;
    pid_t pid;
    void *private_data;
};

struct fuse_file_info
{
    int flags;

    uint64_t fh;
};

typedef struct _fuse_context fuse_context;

extern fuse_context *fuse_ctx;

int fuse_init (void *user_data);
fuse_context *fuse_get_context();

typedef int (*fuse_fill_dir_t) (void *buf, const char *name,
        const struct stat *stbuf, off_t off);

int fake_fuse_dir_filler (void *buf, const char *name,
        const struct stat *stbuf, off_t off);

#endif
