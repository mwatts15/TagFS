#ifndef QUERY_H
#define QUERY_H
#include "tagdb.h"
#include "types.h"
#include "set_ops.h"

query_t *parse (const char *s);
void act (TagDB *db, query_t *q, gpointer *result, int *type);
// can probably remove the table_id argument
typedef void (*q_fn) (TagDB *db, int argc, gchar **argv, gpointer *result, int *type);
void query_info (query_t *q);
void log_query_info (query_t *q);
result_t *tagdb_query (TagDB *db, const char *query);

gboolean value_lt_sp (gpointer key, gpointer value, gpointer lvalue);
gboolean value_eq_sp (gpointer key, gpointer value, gpointer lvalue);
gboolean value_gt_sp (gpointer key, gpointer value, gpointer lvalue);

extern const char *search_operators[2][4];
extern const char *q_commands[2][8][2];
//q_fn q_functions[2][6];
enum _oper_nums
{
    OP_AND, OP_OR, OP_ANDN,
} oper_nums;

/* these are tags with special meanings in TSPEC */
typedef GList* (*special_tag_fn) (TagDB*); 
#endif /* QUERY_H */
