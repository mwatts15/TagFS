#ifndef QUERY_H
#define QUERY_H
#include "tagdb.h"
#include "types.h"

query_t *parse (const char *s);
void act (tagdb *db, query_t *q, gpointer *result, int *type);
// can probably remove the table_id argument
typedef void (*q_fn) (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type);
result_t *encapsulate (int type, gpointer data);
void query_destroy (query_t *q);
void result_destroy (result_t **r);
void query_info (query_t *q);
void log_query_info (query_t *q);
result_t *tagdb_query (tagdb *db, const char *query);
static const char *q_commands[2][7] = {
    // File table commands
    {
        "REMOVE",
        "HAS_TAGS",
        "CREATE",
        "ADD_TAGS",
        "RENAME",
        "LIST_TAGS",
        NULL
        // Other commands
    },
    // Tag table commands
    {
        "IS_EMPTY",
        "REMOVE",
        "TSPEC",
        "CREATE",
        "RENAME",
        NULL
    },
};
void tagdb_tag_tspec (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type);
#endif /* QUERY_H */
