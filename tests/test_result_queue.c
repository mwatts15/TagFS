#include "result_queue.h"
#include "types.h"
#include <stdlib.h>
#include <stdio.h>

result_t *_encapsulate (int type, gpointer data)
{
    //log_msg("ENCAPSULATING\n");
    result_t *res = malloc(sizeof(result_t));
    res->type = type;
    switch (type)
    {
        case tagdb_dict_t:
            res->data.d = data;
            break;
        case tagdb_int_t:
            res->data.i = GPOINTER_TO_INT(data);
            break;
        case tagdb_str_t:
            res->data.s = data;
            break;
        case tagdb_err_t:
            if (data == NULL)
                res->data.s = NULL;
            else
                res->data.s = data;
            break;
        default:
            res->data.b = data;
    }
    return res;
}

int main ()
{
    ResultQueueManager *rqm = malloc(sizeof(ResultQueueManager));
    rqm->queue_table = g_hash_table_new_full(g_str_hash,
            g_str_equal, (GDestroyNotify) g_free, 
            (GDestroyNotify) g_queue_free);
    result_t *res = encapsulate(tagdb_int_t, 99);
    result_queue_new(rqm, "q1");
    result_queue_add(rqm, "q1", res);
    result_t *qres = result_queue_remove(rqm, "q1");
    char *resstr = tagdb_value_to_str(qres->type, &(qres->data));
    printf("type = %d\nvalue = %s\n", qres->type, resstr);
    return 0;
}
