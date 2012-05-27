#ifndef QUERY_H
#define QUERY_H
#include "tagdb.h"
#include "types.h"
#include "set_ops.h"

query_t *parse (const char *s);
void act (tagdb *db, query_t *q, gpointer *result, int *type);
// can probably remove the table_id argument
typedef void (*q_fn) (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type);
result_t *encapsulate (int type, gpointer data);
void query_info (query_t *q);
void log_query_info (query_t *q);
result_t *tagdb_query (tagdb *db, const char *query);


gboolean value_lt_sp (gpointer key, gpointer value, gpointer lvalue);
gboolean value_eq_sp (gpointer key, gpointer value, gpointer lvalue);
gboolean value_gt_sp (gpointer key, gpointer value, gpointer lvalue);

extern const char *tspec_operators[2][4];
extern const char *q_commands[2][7];
enum _oper_nums
{
    OP_AND, OP_OR, OP_ANDN,
} oper_nums;

/* these are tags with special meanings in TSPEC */
typedef GHashTable* (*special_tag_fn) (tagdb*); 
void tagdb_tag_tspec (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type);
#endif /* QUERY_H */
