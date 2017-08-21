#ifndef GLIB_FUTURE_H
#define GLIB_FUTURE_H
#include <glib.h>
#if GLIB_CHECK_VERSION(2,44,0)
#else
gboolean
g_strv_contains (const gchar * const *strv,
                 const gchar *str);
#endif
#endif /* GLIB_FUTURE_H */

