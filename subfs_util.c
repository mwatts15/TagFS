#include "search_fs.h"
#include "result_fs.h"

struct fuse_operations subsystems[] = {
    tagdb_oper,
    search_oper,
    result_oper,
};


