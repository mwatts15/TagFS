#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <glib.h>

#include "log.h"
#include "util.h"
#include "asp.h"

// tag_counter includes
#include <semaphore.h>
#include "tag.h"

GHashTable *aspect_parts;
GHashTable *aspect_data;
GHashTable *aspect_destroys;
GHashTable *aspect_inits;
GHashTable *aspect_ids;

typedef struct {
    GHashTable *counters;
    sem_t counter_sema;
} tag_counts_data;

ASPInit(tag_counts, 0x1)
{
    tag_counts_data *udata = malloc(sizeof(tag_counts_data));
    sem_init(&udata->counter_sema, 0, 1);
    udata->counters = g_hash_table_new(g_direct_hash, g_direct_equal);
    return udata;
} ASPInit_END

ASPDestroy(tag_counts, tag_counts_data *data)
{
    sem_destroy(&data->counter_sema);
    g_hash_table_destroy(data->counters);
    free(data);
} ASPDestroy_END

ASP(tag_counts, tag_destroy1, uint32_t, ret, ret_destroy, tag_counts_data *udata, args)
{
    (void)va_arg(args, ASPPoint);
    Tag** tag = va_arg(args, Tag**);
    sem_wait(&udata->counter_sema);
    gpointer val = g_hash_table_lookup(udata->counters, (*tag)->default_value);
    if (val)
    {
        fprintf(stderr, "Attempted to twice destroy a tag's default value\n");
        abort();
    }
    g_hash_table_insert(udata->counters, (*tag)->default_value, TO_PP(1));
    sem_post(&udata->counter_sema);
    return ASP_RET_PERSIST;
} ASP_END

ASPm(tag_counts, tag_destroy1, ASP_TAG_DESTROY1);

void asp_register_aspect_part (ASPFunc fn, ASPPoint point_id, ASPInitFunc init,
        ASPDestroyFunc destr, ASPId asp_id)
{
    HUPL(aspect_parts, TO_PP(point_id), fn);
    // TODO Consider making a struct to contain these...
    g_hash_table_insert(aspect_inits, TO_PP(asp_id), init);
    g_hash_table_insert(aspect_destroys, TO_PP(asp_id), destr);
    g_hash_table_insert(aspect_ids, fn, TO_PP(asp_id));
}

void asp_init()
{
    aspect_parts = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL,
            (GDestroyNotify)g_list_free);
    aspect_inits = g_hash_table_new(g_direct_hash, g_direct_equal);
    aspect_destroys = g_hash_table_new(g_direct_hash, g_direct_equal);
    aspect_data = g_hash_table_new(g_direct_hash, g_direct_equal);
    aspect_ids = g_hash_table_new(g_direct_hash, g_direct_equal);

    ASPReg(tag_counts, tag_destroy1);
}

void asp_destroy()
{
    HL(aspect_data, it, k, v)
    {
        ASPDestroyFunc destroy = g_hash_table_lookup(aspect_destroys, k);
        LL(v, itt)
        {
            destroy(itt->data);
        } LL_END
        g_list_free(v);
    } HL_END
    g_hash_table_destroy(aspect_parts);
    g_hash_table_destroy(aspect_inits);
    g_hash_table_destroy(aspect_destroys);
    g_hash_table_destroy(aspect_ids);
    g_hash_table_destroy(aspect_data);
}

void asp_execute(ASPPoint point_id, void **ret, ASPRetDestroyFunc *ret_destroy, ...)
{
    GList *execs = g_hash_table_lookup(aspect_parts, TO_PP(point_id));
    va_list ap;
    LL(execs, it)
    {
        ASPFunc func = (ASPFunc)it->data;
        gpointer asp_id = g_hash_table_lookup(aspect_ids, func);

        GList *datas = g_hash_table_lookup(aspect_data, asp_id);
        if (!datas)
        {
            ASPInitFunc init_func = g_hash_table_lookup(aspect_inits, asp_id);
            gpointer data = init_func();
            HUPL(aspect_data, asp_id, data);
            datas = g_hash_table_lookup(aspect_data, asp_id);
        }
        GList *survivors = NULL;
        LL(datas, itt)
        {
            va_start(ap, ret_destroy);
            gpointer this_data = itt->data;
            int stat = func(ret, ret_destroy, this_data, ap);
            if (!(stat & ASP_RET_PERSIST))
            {
                ASPDestroyFunc destroy_func = g_hash_table_lookup(aspect_destroys, asp_id);
                destroy_func(this_data);
            }
            else
            {
                survivors = g_list_append(survivors, this_data);
            }

            if (ret && *ret != NULL)
            {
                va_end(ap);
                return;
            }
            va_end(ap);
        } LL_END
        g_hash_table_insert(aspect_data, asp_id, survivors);
        g_list_free(datas);
    } LL_END
}
