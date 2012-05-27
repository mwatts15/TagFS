#ifndef PARAMS_H
#define PARAMS_H
#include <stdio.h>
#include "tagdb.h"
#include "result_queue.h"

struct tagfs_state {
    char *copiesdir;
    char *mountdir;
    char *listen;
    tagdb *db;
    ResultQueueManager *rqm;
    gboolean debug;
};

#define TAGFS_DATA ((struct tagfs_state *) fuse_get_context()->private_data)

#endif /* PARAMS_H */

