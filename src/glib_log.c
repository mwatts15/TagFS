#include "log.h"

void glib_log_handler (const char *message)
{
    log_msg1(g_log_filtering_level, __FILE__, __LINE__, "GLIB:\n%s", message);
}
