#ifndef TYPES_H
#define TYPES_H

// enumeration of types in result_t
enum _tagdb_types
{
    tagdb_dict_t,
    tagdb_int_t,
    tagdb_str_t,
} tagdb_types;

union result_d
{
    GHashTable *d;
    int i;
    char *s;
    gpointer b;
};

struct query_t
{
    int table_id;
    int command_id;
    int argc;
    gchar* argv[256];
};

struct result_t
{
    int type;
    union result_d data;
};

typedef struct query_t query_t;
typedef struct result_t result_t;

#endif /* TYPES_H */
