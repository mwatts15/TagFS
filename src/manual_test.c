#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include "tagfs_common.h"
#include "params.h"
#include "util.h"

int main (int argc, char **argv)
{
    struct tagfs_state *data = process_options0(&argc, &argv, 0);
    fake_fuse_init(data);
    struct fuse_file_info finfo;
    finfo.flags = 0;
    tagfs_operations_oper.create("/some_file", 0777, &finfo);
    tagfs_operations_oper.create("/some_other_file", 0777, &finfo);
    tagfs_operations_oper.destroy(data);
    return 0;
}

