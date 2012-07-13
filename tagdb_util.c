#include "log.h"
#include "tagdb_util.h"

void print_key (tagdb_key_t k)
{
    log_msg("<<");
    KL(k, i)
    {
        log_msg("%ld ", k[i]);
    } KL_END;
    log_msg(">>\n");
}

