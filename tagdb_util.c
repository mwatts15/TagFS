#include "log.h"
#include "tagdb_util.h"

void print_key (gulong *k)
{
    log_msg("<<");
    KL(k, i)
        log_msg("%ld ", k[i]);
    KL_END(k, i);
    log_msg(">>\n");
}

