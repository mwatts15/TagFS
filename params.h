#ifndef PARAMS_H
#define PARAMS_H
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef TAGFS_BUILD

#ifdef FUSE_USE_VERSION
#undef FUSE_USE_VERSION
#endif

#define __need_timespec
#define FUSE_USE_VERSION 26
#include <fuse.h>

#else
#include <stdio.h>

#endif /* TAGFS_BUILD */

#include "tagfs.h"

#define FSDATA ((struct tagfs_state *) fuse_get_context()->private_data)

#define DB FSDATA->db
#define STAGE FSDATA->stage

/* Default permissions for directories */
#define DIR_PERMS 0755 | S_IFDIR

/* The prefix for xattr presentation of tags */
#define XATTR_PREFIX "user.tagfs."

/* For marco.pl to know not to put a %s in the format for a function
 * prototype log */
#define __DNP__
#include "version.h"
#endif /* PARAMS_H */

