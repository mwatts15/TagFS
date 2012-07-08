#ifndef PARAMS_H
#define PARAMS_H
#include <stdio.h>
#ifdef FUSE_USE_VERSION
#undef FUSE_USE_VERSION
#endif

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "tagdb.h"
#include "result_queue.h"
#include "stage.h"

struct tagfs_state {
    char *copiesdir;
    char *mountdir;
    char *log_file;
    TagDB *db;
    Stage *stage;
    /* The result from the last search performed
       a list of files */
    result_t *search_result;
    char *search_string;
    ResultQueueManager *rqm;
    gboolean debug;
};

#define FSDATA ((struct tagfs_state *) fuse_get_context()->private_data)
#define DB FSDATA->db
#define STAGE FSDATA->stage
#define TAGFS_BUILD 1
/* This handle alone denotes a file that can be written to to make a query on
   the TagDB. If anything follows the handle, everything after is interpreted
   as the query itself and the type will be dictated by the type of query. */
#define LISTEN_FH "#L"
/* This handle is for reading results written to the listen file handle they
   are given in binary format */
#define QREAD_PREFIX "#R"
/* A sort of shortcut for "#LFILE SEARCH" */
#define SEARCH_PREFIX "#?"
#define UNTAG_FH "#X"

gboolean tagfs_is_consistent ();
void toggle_tagfs_consistency ();
void ensure_tagfs_consistency (struct tagfs_state *st);
#endif /* PARAMS_H */

