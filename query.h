#ifndef QUERY_H
#define QUERY_H
#include "tagdb.h"
#include "types.h"
query_t *parse (const char *s);
int act (tagdb *db, query_t *q, gpointer *result, int *type);
typedef void (*q_fn) (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type);
result_t *encapsulate (tagdb *db, int type, gpointer data);
static const char *q_commands[] = {// Tag table commands
    "IS_EMPTY",
    // File table commands
    "HAS_TAGS",
    NULL};
#endif /* QUERY_H */
