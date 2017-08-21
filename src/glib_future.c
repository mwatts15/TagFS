#include "private/glib_future.h"
gboolean g_strv_contains (const gchar * const *strv, const gchar *str)
{
    int i = 0;
    while (strv[i] != NULL)
    {
        if (g_strcmp0(strv[i], str) == 0)
        {
            return TRUE;
        }
        i++;
    }
    return FALSE;
}
