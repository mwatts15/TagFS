#include <glib.h>
#include "result_queue.h"
#include "types.h"

ResultQueueManager *result_queue_manager_new ()
{
    ResultQueueManager *res = g_malloc(sizeof(ResultQueueManager));
    res->queue_table = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) g_queue_free);
    return res;
}

void result_queue_new (ResultQueueManager *qm, gint64 key)
{
    g_hash_table_insert(qm->queue_table, key, g_queue_new());
}

result_t *result_queue_peek (ResultQueueManager *qm, gint64 key)
{
    GQueue *q = g_hash_table_lookup(qm->queue_table, key);
    return (result_t*) g_queue_peek_head(q);
}

result_t *result_queue_remove (ResultQueueManager *qm, gint64 key)
{
    GQueue *q = g_hash_table_lookup(qm->queue_table, key);
    return (result_t*) g_queue_pop_head(q);
}

void result_queue_add (ResultQueueManager *qm, gint64 key, result_t *res)
{
    if (!result_queue_exists(qm, key))
    {
        result_queue_new(qm, key);
    }
    GQueue *q = g_hash_table_lookup(qm->queue_table, key);
    g_queue_push_tail(q, res);
}

gboolean result_queue_exists (ResultQueueManager *qm, gint64 key)
{
    GQueue *q = g_hash_table_lookup(qm->queue_table, key);
    return (q != NULL);
}
