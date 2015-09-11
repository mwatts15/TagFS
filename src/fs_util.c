#include <fuse.h>
#include <unistd.h>
#include "fs_util.h"
#include "file_log.h"
#include "log.h"

/** Utils common to more than one FS such as reading from a file whose descriptor
 * is stored in f_info, or wrapping an object so that it has a `struct stat` */

struct FWrapper {
    struct stat statbuf;
    void *data;
    GDestroyNotify data_destroy;
};

int file_info_read (struct fuse_file_info *f_info, char *buffer, size_t size, off_t offset)
{
    /* This also works for the file search since we've opened the file */
    log_fi(f_info);

    return pread(f_info->fh, buffer, size, offset);
}

int file_info_write (struct fuse_file_info *f_info, const char *buf, size_t size, off_t offset)
{
    log_fi(f_info);

    return pwrite(f_info->fh, buf, size, offset);
}

int file_info_fsync (struct fuse_file_info *f_info, int datasync)
{
    int retstat = 0;

    log_fi(f_info);

    if (datasync)
        retstat = fdatasync(f_info->fh);
    else
        retstat = fsync(f_info->fh);

    if (retstat < 0)
        log_error("tagfs_fsync fsync");

    return retstat;
}

int file_info_truncate (struct fuse_file_info *f_info, off_t size)
{
    return ftruncate(f_info->fh, size);
}

FWrapper *fwrapper_wrap (void *data)
{
    return fwrapper_wrap_full(data, NULL);
}

FWrapper *fwrapper_wrap_full (void *data, GDestroyNotify data_destroy)
{
    FWrapper *fw = g_malloc0(sizeof(struct FWrapper));
    fw->data = data;
    fw->data_destroy = data_destroy;
    return fw;
}

void fwrapper_destroy (FWrapper *fw)
{
    if (fw->data_destroy)
    {
        fw->data_destroy(fw->data);
        fw->data = NULL;
    }
    g_free(fw);
}

struct stat *fwrapper_stat(FWrapper *fw)
{
    return &(fw->statbuf);
}

void *fwrapper_get_data(FWrapper *fw)
{
    return fw->data;
}
