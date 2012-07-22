#include <glib.h>
#include "query_fs_result_manager.h"
#include "types.h"

/* The manager stores the queries made in the file system by the query string.
 * Because we're responsible for the cleaning up the keys we passed and those
 * keys are also the queries to be made, we also make the queries on insert.
 */
static GHashTable *query_result_manager_queries_new ()
{
    return g_hash_table_new_full(g_str_hash, g_str_equal,
            g_free, (GDestroyNotify) result_destroy);
}

QueryResultManager *query_result_manager_new ()
{
    QueryResultManager *res = g_malloc(sizeof(QueryResultManager));
    res->queries = query_result_manager_queries_new();
    return res;
}

result_t *query_result_lookup (QueryResultManager *qm, char *key)
{
    return g_hash_table_lookup(qm->queries, key);
}

void query_result_remove (QueryResultManager *qm, char *key)
{
    g_hash_table_remove(qm->queries, key);
}

void query_result_insert (QueryResultManager *qm, char *key, result_t *r)
{
    g_hash_table_insert(qm->queries, g_strdup(key), r);
}
