#ifndef PARAMS_H
#define PARAMS_H
#include <fuse.h>
#include "tagdb.h"

struct tagfs_state {
    char *copiesdir;
    char *mountdir;
    char *listen;
    tagdb *db;
};

#define TAGFS_DATA ((struct tagfs_state *) fuse_get_context()->private_data)

#endif /* PARAMS_H */

