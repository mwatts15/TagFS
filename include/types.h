#ifndef TYPES_H
#define TYPES_H
#include <glib.h>

#define TAGDB_DICT_TYPE 0
#define TAGDB_LIST_TYPE 1
#define TAGDB_INT_TYPE 2
#define TAGDB_STR_TYPE 3
#define TAGDB_BIN_TYPE 4
#define TAGDB_INT64_TYPE 5
#define TAGDB_ERR_TYPE = -1

typedef char tagdb_value_t;
typedef char result_t;
const char *default_value(int type);

#define result_destroy g_free
#define copy_value(__x) g_strdup((__x))

#endif /* TYPES_H */

