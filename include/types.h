#ifndef TYPES_H
#define TYPES_H
#include <glib.h>
enum _tagdb_types
{
    TAGDB_DICT_TYPE,
    TAGDB_LIST_TYPE,
    TAGDB_INT_TYPE,
    TAGDB_STR_TYPE,
    TAGDB_BIN_TYPE,
    TAGDB_INT64_TYPE,
    TAGDB_ERR_TYPE = -1,
} tagdb_type;
typedef char tagdb_value_t;
typedef char result_t;
const char *default_value(int type);
#define result_destroy g_free
#define copy_value(__x) g_strdup((__x))
#endif /* TYPES_H */

