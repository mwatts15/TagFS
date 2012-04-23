#ifndef TYPES_H
#define TYPES_H
#include <glib.h>

// enumeration of types in result_t
enum _tagdb_types
{
    tagdb_dict_t,
    tagdb_list_t,
    tagdb_int_t,
    tagdb_str_t,
    tagdb_err_t = -1,
} tagdb_types;

// used in queries, but possibly elsewhere too
static const char *type_strings[] = {
    "DICT",
    "LIST",
    "INT",
    "STRING"
};

// Values returned in by a query
// Also used in the database for tag values
union tagdb_value
{
    GHashTable *d;
    GList *l;
    int i;
    char *s;
    gpointer b;
};

struct query_t
{
    int table_id;
    int command_id;
    int argc;
    gchar* argv[10];
};

struct result_t
{
    int type;
    union tagdb_value data;
};

typedef struct query_t query_t;
typedef struct result_t result_t;

union tagdb_value *tagdb_str_to_value(int type, char *data);
char              *tagdb_value_to_str (int type, union tagdb_value *value);
#endif /* TYPES_H */
