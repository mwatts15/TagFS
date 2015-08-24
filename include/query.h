#ifndef QUERY_H
#define QUERY_H
#include "tagdb.h"
#include "types.h"
#include "set_ops.h"

query_t *parse (const char *s);
void act (TagDB *db, query_t *q, gpointer *result, char **type);
// can probably remove the table_id argument
typedef int (*q_fn) (TagDB *db, int argc, gchar **argv, gpointer *result, char **type);
void query_info (query_t *q);
void log_query_info (query_t *q);
int check_argc (int argc, int required, gpointer *result, char **type);
result_t *tagdb_query (TagDB *db, const char *query);

#define check_args(num) \
    if (check_argc(argc, num, result, type) == -1) \
        return -1;

#endif /* QUERY_H */
