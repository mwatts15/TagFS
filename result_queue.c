#include <glib.h>
#include "result_queue.h"
#include "types.h"

void result_queue_new (ResultQueueManager *qm, const char *name)
{
    g_hash_table_insert(qm->queue_table, g_strdup(name), g_queue_new());
}

result_t *result_queue_peek (ResultQueueManager *qm, const char *name)
{
    GQueue *q = g_hash_table_lookup(qm->queue_table, name);
    return (result_t*) g_queue_peek_head(q);
}

result_t *result_queue_remove (ResultQueueManager *qm, const char *name)
{
    GQueue *q = g_hash_table_lookup(qm->queue_table, name);
    return (result_t*) g_queue_pop_head(q);
}

void result_queue_add (ResultQueueManager *qm, const char *name, result_t *res)
{
    GQueue *q = g_hash_table_lookup(qm->queue_table, name);
    g_queue_push_tail(q, res);
}

gboolean result_queue_exists (ResultQueueManager *qm, const char *name)
{
    GQueue *q = g_hash_table_lookup(qm->queue_table, name);
    return (q != NULL);
}
