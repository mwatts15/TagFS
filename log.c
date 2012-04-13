// Since the point of this filesystem is to learn FUSE and its
// datastructures, I want to see *everything* that happens related to
// its data structures.  This file contains macros and functions to
// accomplish this.

#include "params.h"
#include "query.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <fuse.h>

#include "log.h"

FILE *log_open(const char *name)
{
    FILE *logfile;
    
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

    vfprintf(TAGFS_DATA->logfile, format, ap);
}
    
void log_query_info (query_t *q)
{
    if (q==NULL)
    {
        log_msg("query_info: got q==NULL\n");
        return;
    }
    log_msg("query info:\n");
    log_msg("\ttable_id: %s\n", (q->table_id==FILE_TABLE)?"FILE_TABLE":"TAG_TABLE");
    log_msg("\tcommand: %s\n", q_commands[q->table_id][q->command_id]);
    log_msg("\targc: %d\n", q->argc);
    int i;
    for (i = 0; i < q->argc; i++)
    {
        log_msg("\targv[%d] = %s\n", i, q->argv[i]);
    }
}

void log_pair (gpointer key, gpointer val, gpointer not_used)
{
    log_msg("%p=>",  key);
    log_msg("%p ", val);
}

void log_hash (GHashTable *hsh)
{
    log_msg("{");
    if (hsh != NULL)
        g_hash_table_foreach(hsh, log_pair, NULL);
    log_msg("}");
    log_msg("\n");
}

// struct fuse_file_info keeps information about files (surprise!).
// This dumps all the information in a struct fuse_file_info.  The struct
// definition, and comments, come from /usr/include/fuse/fuse_common.h
// Duplicated here for convenience.
void log_fi (struct fuse_file_info *fi)
{
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

    /** File handle.  May be filled in by filesystem in open().
        Available in all other file operations */
    //	uint64_t fh;
	log_struct(fi, fh, 0x%016llx,  );
	
    /** Lock owner id.  Available in locking operations and flush */
    //  uint64_t lock_owner;
	log_struct(fi, lock_owner, 0x%016llx, );
};

// This dumps the info from a struct stat.  The struct is defined in
// <bits/stat.h>; this is indirectly included from <fcntl.h>
void log_stat(struct stat *si)
{
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
	
}

void log_statvfs(struct statvfs *sv)
{
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
	
}

void log_utime(struct utimbuf *buf)
{
	//    time_t actime;
	log_struct(buf, actime, 0x%08lx, );
	
	//    time_t modtime;
	log_struct(buf, modtime, 0x%08lx, );
}
