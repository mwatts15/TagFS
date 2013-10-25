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
#define SEARCHES FSDATA->search_results

/* Default permissions for directories */
#define DIR_PERMS 0755 | S_IFDIR

#define LEADER ":"
/* This handle alone denotes a file that can be written to to make a query on
   the TagDB. If anything follows the handle, everything after is interpreted
   as the query itself and the type will be dictated by the type of query. */
#define LISTEN_FH LEADER "Q"
/* This handle is for reading results written to the listen file handle they
   are given in binary format */
#define QREAD_PREFIX LEADER "R"
/* Handle for integrated file search */
#define SEARCH_PREFIX LEADER "S"
/* This handle alone denotes a directory which, if a file is moved there will,
 * remove all of the tags preceding it in the list. Listing this contents of
 * directory will list the files which do NOT have the preceding tags along
 * with the union of those files tags as per usual. If anything follows the
 * handle, it will be interpreted as a single tag name and the file will
 * resolve to a directory the contents of which contain all files in the
 * preceding path, but excluding the tag following the handle. */
#define UNTAG_FH LEADER "X"

/* This handle provides a file (or directory, haven't decided) that gives some
 * information about the tag or file that follows it. Only names in the same
 * directory will be resolved, to prevent name conflicts. Symbolic directory
 * names . and .. are read as well. */
#define INFO_FH LEADER "I"

/* If any file begins with the leader, it will be read in as normal so long as
 * the leader does not contiune into one of the above handles */

/* For lex.pl to know not to print out a character buffer */
#define __DNP__
#endif /* PARAMS_H */

