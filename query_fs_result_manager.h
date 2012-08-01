#ifndef QUERY_RESULT_MANAGER_H
#define QUERY_RESULT_MANAGER_H
#include <glib.h>
#include "types.h"

typedef struct
{
    /* This is the result from a query written to the listen
     * file. The result, regardless of it's type is stored as
     * a binstring*/
    result_t *file_contents;
    GHashTable *queries;
} QueryResultManager;

QueryResultManager *query_result_manager_new ();

void query_result_store_contents (QueryResultManager *qm, result_t *r);
result_t *query_result_retrieve_contents (QueryResultManager *qm);

void query_result_remove (QueryResultManager *qm, char *key);
result_t *query_result_lookup (QueryResultManager *qm, char *key);
void query_result_insert (QueryResultManager *qm, char *key, result_t *r);
#endif
