#include "query.h"
#include "util.h"
#include "tokenizer.h"
#include "set_ops.h"
// query api for tagdb
// parses query strings and calls the appropriate methods from tagdb
// also handles conversions from the external strings to internal ids
// when necessary

// Functions must return the type of their result (as enumerated in types.h) and the result itself
// the function returns these in the two result arguments it is given
// functions must accept an argument count and an argument list (NULL-terminated array of gpointers)
// finally, functions will take a pointer to a tagdb and a table id as named arguments,
// thus functions will have the form:
// void (*func) (tagdb *db, int table_id, int argc, char **argv, gpointer result, int type)
void tagdb_tag_is_empty (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type)
{
    if (argc < 1)
        return;
    int tcode = tagdb_get_tag_code(db, argv[0]);
    GHashTable *tmp = tagdb_get_item(db, tcode, table_id);
    if (tmp != NULL)
    {
        *result = GINT_TO_POINTER((g_hash_table_size(tmp) == 0)?TRUE:FALSE);
        *type = tagdb_int_t;
    }
    else
    {
        *result = GINT_TO_POINTER(TRUE);
        *type = tagdb_int_t;
    }
}

void tagdb_file_has_tags (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type)
{
    if (argc < 1)
        return;
    GHashTable *tmp = tagdb_get_item(db, atoi((char*) argv[0]), table_id);
    int i;
    int tcode;
    argc--;
    argv++;
    *type = tagdb_int_t;
    for (i = 0; i < argc; i++)
    {
        tcode = tagdb_get_tag_code(db, argv[i]);
        if (g_hash_table_lookup(tmp, GINT_TO_POINTER(tcode)) == NULL)
        {
            *result = GINT_TO_POINTER(FALSE);
            return;
        }
    }
    *result = GINT_TO_POINTER(TRUE);
    return;
}

void tagdb_file_remove (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type)
{
    if (argc < 1)
        return;
    tagdb_remove_item(db, atoi(argv[0]), table_id);
}

void tagdb_tag_remove (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type)
{
    if (argc < 1)
        return;
    tagdb_remove_item(db, tagdb_get_tag_code(db, argv[0]), table_id);
}

void tagdb_tag_tspec (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type)
{
    if (argc < 1)
        return;
    GList *seps = g_list_new_charlist('/','\\','~', NULL);
    char c;
    char op;
    char *s;
    Tokenizer *tok = tokenizer_new(seps);
    GHashTable *res = tagdb_get_table(db, TAG_TABLE); // this is our universe
    tokenizer_set_str_stream(tok, argv[0]);

    s = tokenizer_next(tok, &c);
    op = -1;
    while (s != NULL)
    {
        int n = tagdb_get_tag_code(db, s);
        printf("op=%c\n c=%c\n s=%s\n", op, c, s);
        if (n != 0)
        {
            GHashTable *tab = tagdb_get_item(db, n, TAG_TABLE);
            GHashTable *tmp = res;
            if (op == '/') // intersection
            {
                res = set_intersect_s(res, tab);
            }
            if (op == '\\') // union
            {
                res = set_union_s(res, tab);
            }
            if (op == '~') // intersection
            {
                res = set_difference_s(res, tab);
            }
        }
        g_free(s);
        op = c;
        s = tokenizer_next(tok, &c);
    }
    *result = res;
    *type = tagdb_dict_t;
}
q_fn q_functions[2][3] = {// Tag table funcs
    {
        tagdb_file_remove,
        tagdb_file_has_tags,
    },
    {
        tagdb_tag_is_empty,
        tagdb_tag_remove,
        tagdb_tag_tspec
    }
};

// encode the command name
int _name_to_code (const char *name, int table_id)
{
    // remember, lists and iteration solve everything!
    int i = 0;
    while (q_commands[table_id][i] != NULL)
    {
        if (g_strcmp0(q_commands[table_id][i], name) == 0)
            return i;
        i++;
    }
    return -1;
}


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
// parse (string) -> query_t
query_t *parse (const char *s)
{
    query_t *qr = malloc(sizeof(query_t));
    char *qs = g_strstrip(g_strdup(s));
    char sep;
    char *token;
    GList *seps = g_list_new_charlist(' ', NULL);
    Tokenizer *tok = tokenizer_new(seps);
    tokenizer_set_str_stream(tok, qs);
    token = tokenizer_next(tok, &sep);
    if (g_strcmp0(token, "FILE") == 0)
    {
        qr->table_id = 0;
    }
    if (g_strcmp0(token, "TAG") == 0)
    {
        qr->table_id = 1;
    }
    g_free(token);
    token = tokenizer_next(tok, &sep);
    qr->command_id = _name_to_code(token, qr->table_id);
    g_free(token);
    token = tokenizer_next(tok, &sep);
    int i = 0;
    while (token != NULL && i < 255)
    {
        qr->argv[i] = token;
        i++;
        token = tokenizer_next(tok, &sep);
    }
    qr->argv[i] = NULL;
    qr->argc = i;
    return qr;
}

// action takes a compiled query and performs the action on the database db
// query
int act (tagdb *db, query_t *q, gpointer *result, int *type)
{
    q_functions[q->table_id][q->command_id](db, q->table_id, q->argc, q->argv, result, type);
}
// -> (type, *object*)
// encapsualtes the object in a result type
// with info about the type based on type
// we pass the database because encapsulation may make further
// queries to acquire data that the user expects
result_t *encapsulate (tagdb *db, int type, gpointer data)
{
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
        default:
            res->data.b = data;
    }
    return res;
}
