// Since the point of this filesystem is to learn FUSE and its
// datastructures, I want to see *everything* that happens related to
// its data structures.  This file contains macros and functions to
// accomplish this.


#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/file.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include <inttypes.h>

#include "params.h"
#include "log.h"
#include "file_log.h"

// struct fuse_file_info keeps information about files (surprise!).
// This dumps all the information in a struct fuse_file_info.  The struct
// definition, and comments, come from /usr/include/fuse/fuse_common.h
// Duplicated here for convenience.
void log_fi (struct fuse_file_info *fi)
{
#pragma GCC diagnostic push

/* Many of these formats are wrong for my system, but I can't be bothered to fix
 * them.
 */
#pragma GCC diagnostic ignored "-Wformat"

    lock_log();
#ifdef TAGFS_BUILD
    /** Open flags.  Available in open() and release() */
    //	int flags;
    log_struct(fi, flags, 0x%08x, );

    /** Old file handle, don't use */
    //	unsigned long fh_old;
    log_struct(fi, fh_old, 0x%08lx,  );

    /** In case of a write operation indicates if this was caused by a
      writepage */
    //	int writepage;
    log_struct(fi, writepage, %d, );

    /** Can be filled in by open, to use direct I/O on this file.
      Introduced in version 2.4 */
    //	unsigned int keep_cache : 1;
    log_struct(fi, direct_io, %d, );

    /** Can be filled in by open, to indicate, that cached file data
      need not be invalidated.  Introduced in version 2.4 */
    //	unsigned int flush : 1;
    log_struct(fi, keep_cache, %d, );

    /** Padding.  Do not use*/
    //	unsigned int padding : 29;

#endif
    /** File handle.  May be filled in by filesystem in open().
      Available in all other file operations */
    //	uint64_t fh;
    log_struct(fi, fh, 0x%016llx,  );

#ifdef TAGFS_BUILD
    /** Lock owner id.  Available in locking operations and flush */
    //  uint64_t lock_owner;
    log_struct(fi, lock_owner, 0x%016llx, );
#endif
    unlock_log();
#pragma GCC diagnostic pop
};

// This dumps the info from a struct stat.  The struct is defined in
// <bits/stat.h>; this is indirectly included from <fcntl.h>
void log_stat(struct stat *si)
{
#pragma GCC diagnostic push

/* Many of these formats are wrong for my system, but I can't be bothered to fix
 * them.
 */
#pragma GCC diagnostic ignored "-Wformat"

    lock_log();
    //  dev_t     st_dev;     /* ID of device containing file */
    log_struct(si, st_dev, %lld, );

    //  ino_t     st_ino;     /* inode number */
    log_struct(si, st_ino, %lld, );

    //  mode_t    st_mode;    /* protection */
    log_struct(si, st_mode, 0%o, );

    //  nlink_t   st_nlink;   /* number of hard links */
    log_struct(si, st_nlink, %d, );

    //  uid_t     st_uid;     /* user ID of owner */
    log_struct(si, st_uid, %d, );

    //  gid_t     st_gid;     /* group ID of owner */
    log_struct(si, st_gid, %d, );

    //  dev_t     st_rdev;    /* device ID (if special file) */
    log_struct(si, st_rdev, %lld,  );

    //  off_t     st_size;    /* total size, in bytes */
    log_struct(si, st_size, %lld,  );

    //  blksize_t st_blksize; /* blocksize for filesystem I/O */
    log_struct(si, st_blksize, %ld,  );

    //  blkcnt_t  st_blocks;  /* number of blocks allocated */
    log_struct(si, st_blocks, %lld,  );

    //  time_t    st_atime;   /* time of last access */
    log_struct(si, st_atime, 0x%08lx, );

    //  time_t    st_mtime;   /* time of last modification */
    log_struct(si, st_mtime, 0x%08lx, );

    //  time_t    st_ctime;   /* time of last status change */
    log_struct(si, st_ctime, 0x%08lx, );

    unlock_log();
#pragma GCC diagnostic pop
}

void log_statvfs(struct statvfs *sv)
{
#pragma GCC diagnostic push

/* Many of these formats are wrong for my system, but I can't be bothered to fix
 * them.
 */
#pragma GCC diagnostic ignored "-Wformat"
    lock_log();
    //  unsigned long  f_bsize;    /* file system block size */
    log_struct(sv, f_bsize, %ld, );

    //  unsigned long  f_frsize;   /* fragment size */
    log_struct(sv, f_frsize, %ld, );

    //  fsblkcnt_t     f_blocks;   /* size of fs in f_frsize units */
    log_struct(sv, f_blocks, %lld, );

    //  fsblkcnt_t     f_bfree;    /* # free blocks */
    log_struct(sv, f_bfree, %lld, );

    //  fsblkcnt_t     f_bavail;   /* # free blocks for non-root */
    log_struct(sv, f_bavail, %lld, );

    //  fsfilcnt_t     f_files;    /* # inodes */
    log_struct(sv, f_files, %lld, );

    //  fsfilcnt_t     f_ffree;    /* # free inodes */
    log_struct(sv, f_ffree, %lld, );

    //  fsfilcnt_t     f_favail;   /* # free inodes for non-root */
    log_struct(sv, f_favail, %lld, );

    //  unsigned long  f_fsid;     /* file system ID */
    log_struct(sv, f_fsid, %ld, );

    //  unsigned long  f_flag;     /* mount flags */
    log_struct(sv, f_flag, 0x%08lx, );

    //  unsigned long  f_namemax;  /* maximum filename length */
    log_struct(sv, f_namemax, %ld, );

    unlock_log();
#pragma GCC diagnostic pop
}

void log_utime(struct utimbuf *buf)
{
    lock_log();
    //    time_t actime;
    log_struct(buf, actime, 0x%08lx, );

    //    time_t modtime;
    log_struct(buf, modtime, 0x%08lx, );
    unlock_log();
}
