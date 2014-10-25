#ifndef TYPES_H
#define TYPES_H
#include <glib.h>
enum _tagdb_types
{
    tagdb_dict_t,
    tagdb_list_t,
    tagdb_int_t,
    tagdb_str_t,
    tagdb_bin_t,
    tagdb_err_t = -1,
} tagdb_types;
typedef char tagdb_value_t;
typedef char result_t;
const char *default_value(int type);
#define result_destroy g_free
#define copy_value(__x) g_strdup((__x))
#endif /* TYPES_H */

