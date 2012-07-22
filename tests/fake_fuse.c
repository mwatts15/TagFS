#include <glib.h>
#include "log.h"
#include "fake_fuse.h"

fuse_context *fuse_ctx = NULL;

int fuse_init (void *user_data)
{
    fuse_ctx = g_malloc(sizeof(fuse_context));
    fuse_ctx->private_data = user_data;
    return 0;
}

fuse_context *fuse_get_context ()
{
    return fuse_ctx;
}

int fake_fuse_dir_filler (void *buf, const char *name,
        const struct stat *stbuf, off_t off)
{
    log_msg("Filling in %s\n", name);
    return 0;
}
