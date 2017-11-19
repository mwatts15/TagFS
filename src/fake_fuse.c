#include "fake_fuse.h"

struct fuse_context _ctx;

struct fuse_context *fuse_get_context()
{
    return &_ctx;
}

void fake_fuse_init(void *data)
{
    _ctx.private_data = data;
}
