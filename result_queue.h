#ifndef RESULT_QUEUE_MANAGER_H
#define RESULT_QUEUE_MANAGER_H
#include <glib.h>
#include "types.h"

struct ResultQueueManager
{
    GHashTable *queue_table;
};

typedef struct ResultQueueManager ResultQueueManager;
void result_queue_new (ResultQueueManager *qm, gint64 key);
result_t *result_queue_peek (ResultQueueManager *qm, gint64 key);
result_t *result_queue_remove (ResultQueueManager *qm, gint64 key);
void result_queue_add (ResultQueueManager *qm, gint64 key, result_t *res);
gboolean result_queue_exists (ResultQueueManager *qm, gint64 key);
ResultQueueManager *result_queue_manager_new ();
#endif
