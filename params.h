#ifndef PARAMS_H
#define PARAMS_H
#include <stdio.h>
#include "tagdb.h"
#include "result_queue.h"

struct tagfs_state {
    char *copiesdir;
    char *mountdir;
    char *listen;
    TagDB *db;
    ResultQueueManager *rqm;
    gboolean debug;
};

#define FSDATA ((struct tagfs_state *) fuse_get_context()->private_data)
#define DB FSDATA->db
#define TAGFS_BUILD 1

#endif /* PARAMS_H */

