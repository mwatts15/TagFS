#include <fuse.h>
#include "log.h"

/* Utils common to more than one FS such as reading
 * from a file handle stored in f_info */
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
    return ftruncate(fi->fh, offset);
}
