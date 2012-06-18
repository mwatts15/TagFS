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

struct tagfs_state {
    char *copiesdir;
    char *mountdir;
    char *listen;
    char *log_file;
    TagDB *db;
    ResultQueueManager *rqm;
    gboolean debug;
};

#define FSDATA ((struct tagfs_state *) fuse_get_context()->private_data)
#define DB FSDATA->db
#define TAGFS_BUILD 1

#endif /* PARAMS_H */

