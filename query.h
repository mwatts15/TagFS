#ifndef QUERY_H
#define QUERY_H
#include "tagdb.h"
#include "types.h"

query_t *parse (const char *s);
int act (tagdb *db, query_t *q, gpointer *result, int *type);
// can probably remove the table_id argument
typedef void (*q_fn) (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type);
result_t *encapsulate (int type, gpointer data);
void query_info (query_t *q);
void log_query_info (query_t *q);
static const char *q_commands[2][4] = {// Tag table commands
    {
        "REMOVE",
        "HAS_TAGS",
        NULL
        // Other commands
    },
    {
        "IS_EMPTY",
        "REMOVE",
        "TSPEC",
        NULL
    }
    // File table commands
};
#endif /* QUERY_H */
