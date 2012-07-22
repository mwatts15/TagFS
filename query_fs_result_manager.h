#ifndef QUERY_RESULT_MANAGER_H
#define QUERY_RESULT_MANAGER_H
#include <glib.h>
#include "types.h"

typedef struct
{
    GHashTable *queries;
} QueryResultManager;

void query_result_remove (QueryResultManager *qm, char *key);
result_t *query_result_lookup (QueryResultManager *qm, char *key);
void query_result_insert (QueryResultManager *qm, char *key, result_t *r);
QueryResultManager *query_result_manager_new ();
#endif
