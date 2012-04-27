#ifndef RESULT_QUEUE_MANAGER_H
#define RESULT_QUEUE_MANAGER_H
#include <glib.h>
#include "types.h"

struct ResultQueueManager
{
    GHashTable *queue_table;
};

typedef struct ResultQueueManager ResultQueueManager;
void result_queue_new (ResultQueueManager *qm, const char *name);
result_t *result_queue_peek (ResultQueueManager *qm, const char *name);
result_t *result_queue_remove (ResultQueueManager *qm, const char *name);
void result_queue_add (ResultQueueManager *qm, const char *name, result_t *res);
gboolean result_queue_exists (ResultQueueManager *qm, const char *name);
#endif
