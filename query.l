/* Query API for TagDB
 *
 * Parses query strings and calls the appropriate methods.
 */
#include <malloc.h>
#include <string.h>
#include "query.h"
#include "types.h"
#include "scanner.h"
#include "set_ops.h"
#include "util.h"
#include "log.h"

#include "queries.c"

static int _log_level = 1;

// checks if the argc is correct and fills the result and type with the appropriate error
int check_argc (int argc, int required, gpointer *result, char **type)
{
    if (argc < required)
    {
        qerr%Too few arguments%;
    }
    return argc;
}

/* Query functions

   Functions must return the type of their result (as enumerated in types.h) and the result itself
   the function returns these in the two result arguments it is given
   functions must accept an argument count and an argument list (NULL-terminated array of strings)
   finally, functions will take a pointer to a TagDB and a table id as named arguments,
   thus functions will have the form:
   */

/* lookup
 * database queries shall be of the form
 * (FILE HAS_TAGS file_id "argument1" OR "argument2" AND ...)
 * or
 * (TAG IS_EMPTY "tag_name")
 * or
 * (FILE TAG_VALUE file_id "tag_name")
 * etc.
 * So, the first atom is a table specifier, the second item is the action on that table
 * and the rest are appropriate arguments to that action.
 * arguments must be quoted
 * and other query atoms are not quoted
 * returns a compiled query object
 */
query_t *parse (const char *s)
{
    log_msg("");
    if (s == NULL)
        return NULL;
    query_t *qr = malloc(sizeof(query_t));
    char *qs = g_strstrip(g_strdup(s));
    char *sep;
    char *token = NULL;
    GList *seps = g_list_new(" ", NULL);
    GList *quotes = g_list_new("\"", NULL);
    Scanner *scn = scanner_new2(seps, quotes);
    scanner_set_str_stream(scn, qs);
    g_free(qs);
    token = scanner_next(scn, &sep);
    const char *class_strings[] = {"FILE", "TAG", NULL};
    qr->class_id = strv_index(class_strings, token);
    if (qr->class_id == -1)
    {
        free(qr);
        return NULL;
    }
    g_free(token);

    token = scanner_next(scn, &sep);
    qr->command_id = strv_index(query_cmdstrs[qr->class_id], token);
    g_free(token);
    if (qr->command_id == -1)
    {
        free(qr);
        return NULL;
    }
    int i = 0;
    while (!scanner_stream_is_empty(scn->stream) && i < MAX_QUERY_ARGS)
    {
        token = scanner_next(scn, &sep);
        qr->argv[i] = token;
        i++;
    }
    scanner_destroy(scn);
    qr->argv[i] = NULL;
    qr->argc = i;
    return qr;
}

// action takes a compiled query and performs the action on the database db
// query
void act (TagDB *db, query_t *q, gpointer *result, char **type)
{
    log_query_info(q);
    query_functions[q->class_id][q->command_id](db, q->argc, q->argv, result, type);
//    log_msg("Exiting act\n");
}

void print_query_info (query_t *q, printer p)
{
    if (q==NULL)
    {
        p("query_info: got q==NULL\n");
        return;
    }
    p("query info:\n");
    p("\tclass: %s\n", query_class_names[ q->class_id ]);
    p("\tcommand: %s\n", query_cmdstrs[q->class_id][q->command_id]);
    p("\targc: %d\n", q->argc);
    int i;
    for (i = 0; i < q->argc; i++)
    {
        p("\targv[%d] = %s\n", i, q->argv[i]);
    }
}

void query_info (query_t *q)
{
    print_query_info(q, (printer)printf);
}

void log_query_info (query_t *q)
{
    lock_log();
    print_query_info(q, log_msg0);
    unlock_log();
}

// does all of the steps for you
result_t *tagdb_query (TagDB *db, const char *query)
{
    gpointer r = NULL;
    query_t *q = parse(query);
    if (q == NULL)
        return tagdb_str_to_value(tagdb_err_t, "Invalid query");
    char *type = NULL;
    act(db, q, &r, &type);
    query_destroy(q);
    result_t *res = encapsulate(type, r);
    g_free(type);
    res_info(res, log_msg0);
    return res;
}
