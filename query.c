#include <malloc.h>
#include "query.h"
#include "util.h"
#include "types.h"
#include "tokenizer.h"
#include "set_ops.h"
// query api for tagdb
// parses query strings and calls the appropriate methods from tagdb
// also handles conversions from the external strings to internal ids
// when necessary

// checks if the argc is correct and fills the result and type with the appropriate error
int check_argc (int argc, int required, gpointer *result, int *type)
{
    if (argc < required)
    {
        *type = tagdb_err_t;
        *result = "too few arguments"; // TODO: make a real error object
        return FALSE;
    }
    return TRUE;
}

// encode a string from a null-terminated list of strings
int _name_to_code (const char *name, const char **table)
{
    // remember, lists and iteration solve everything!
    int i = 0;
    while (table[i] != NULL)
    {
        if (g_strcmp0(table[i], name) == 0)
            return i;
        i++;
    }
    return -1;
}
/* The tagdb doesn't require that a tag or file "name" value 
   not have any of the standard query separators in the name but
   we want to ensure that is the case within the query system.

*/

// Functions must return the type of their result (as enumerated in types.h) and the result itself
// the function returns these in the two result arguments it is given
// functions must accept an argument count and an argument list (NULL-terminated array of gpointers)
// finally, functions will take a pointer to a tagdb and a table id as named arguments,
// thus functions will have the form:
// void (*func) (tagdb *db, int table_id, int argc, char **argv, gpointer result, int type)
void tagdb_tag_is_empty (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type)
{
    if (!check_argc(argc, 1, result, type))
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
    if (!check_argc(argc, 1, result, type))
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
    if (!check_argc(argc, 1, result, type))
        return;
    tagdb_remove_item(db, atoi(argv[0]), table_id);
}

void tagdb_tag_remove (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type)
{
    if (!check_argc(argc, 1, result, type))
        return;
    tagdb_remove_item(db, tagdb_get_tag_code(db, argv[0]), table_id);
}

void tagdb_tag_create (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type)
{
    if (!check_argc(argc, 2, result, type))
        return;
    int our_type = _name_to_code(argv[1], type_strings);
    if (our_type == tagdb_err_t)
    {
        *type = tagdb_err_t;
        return;
    }
    int res = tagdb_insert_item(db, argv[0], NULL, table_id);
    tagdb_set_tag_type(db, argv[0], our_type);
    *type = tagdb_int_t;
    *result = TO_P(res);
}

void tagdb_file_add_tags (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type)
{
    if(!check_argc(argc, 2, result, type)) // if we aren't given at least a file and a tag, change nothing
        return;
    
    // we shouldn't create a file
    int file_id = atoi(argv[0]);
    if (tagdb_get_item(db, file_id, FILE_TABLE) == NULL)
    {
        *result = 0;
        *type = tagdb_int_t;
        return;
    }
    argc--;
    argv++;

    GHashTable *tags = g_hash_table_new(g_direct_hash, g_direct_equal);

    int i;
    int tagcode;
    int tagtype;
    union tagdb_value *value = NULL;
    gchar **tag_val_pair = NULL;
    for (i = 0; i < argc; i++)
    {
        tag_val_pair = g_strsplit(argv[i], ":", 2);
        // if the tag doesn't already exist we won't create it, but silently ignore
        tagcode = tagdb_get_tag_code(db, tag_val_pair[0]);
        if (tagcode > 0)
        {
            tagtype = tagdb_get_tag_type_from_code(db, tagcode);
            value = tagdb_str_to_value(tagtype, tag_val_pair[1]);
            g_hash_table_insert(tags, GINT_TO_POINTER(tagcode), value);
        }
    }
    *type = tagdb_int_t;
    *result = TO_P(tagdb_insert_item(db, TO_P(file_id), tags, FILE_TABLE));
}
// Returns the id of the file created
void tagdb_file_create (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type)
{
    if (argc == 0)
    {
        *type = tagdb_int_t;
        *result = TO_P(tagdb_insert_item(db, NULL, NULL, FILE_TABLE));
        return;
    }

    GHashTable *tags = g_hash_table_new(g_direct_hash, g_direct_equal);

    int i;
    int tagcode;
    int tagtype;
    union tagdb_value *value = NULL;
    gchar **tag_val_pair = NULL;
    for (i = 0; i < argc; i++)
    {
        tag_val_pair = g_strsplit(argv[i], ":", 2);
        // if the tag doesn't already exist we won't create it, but silently ignore
        tagcode = tagdb_get_tag_code(db, tag_val_pair[0]);
        if (tagcode > 0)
        {
            tagtype = tagdb_get_tag_type_from_code(db, tagcode);
            value = tagdb_str_to_value(tagtype, tag_val_pair[1]);
            g_hash_table_insert(tags, GINT_TO_POINTER(tagcode), value);
        }
    }
    *type = tagdb_int_t;
    *result = TO_P(tagdb_insert_item(db, NULL, tags, FILE_TABLE));
}

// value is [int type, union tagdb_value *value]
gboolean value_equals (gpointer k, gpointer v, gpointer value)
{
    int type = TO_I(((gpointer*) value)[0]);
    union tagdb_value *rhs = ((gpointer*) value)[1];
    //log_msg("v == %s\n", v);
    union tagdb_value *lhs = (union tagdb_value*) v;

    switch (type)
    {
        case (tagdb_dict_t):
        case (tagdb_list_t):
            return FALSE;
        case (tagdb_int_t):
            return (lhs->i == rhs->i);
        case (tagdb_str_t):
            return (g_strcmp0(lhs->s, rhs->s) == 0);
        default:
            return FALSE;
    }
    return FALSE;
}

void tagdb_tag_tspec (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type)
{
    if (!check_argc(argc, 1, result, type))
        return;
    if (g_strcmp0(argv[0], "/") == 0)
    {
        *result = tagdb_files(db);
        *type = tagdb_dict_t;
        return;
    }

    GList *seps = g_list_new_charlist('/','\\','~','=', NULL);
    char c;
    char op;
    char *s = NULL;
    Tokenizer *tok = tokenizer_new(seps);
    GHashTable *r = tagdb_get_table(db, FILE_TABLE); // this is our universe
    tokenizer_set_str_stream(tok, argv[0]);

    s = tokenizer_next(tok, &c);
    op = -1;
    //log_msg("s = \"%s\"\n", s);
    while (s != NULL )
    {
        //log_msg("op=%c\n c=%c\n s=%s\n", op, c, s);
        int n = tagdb_get_tag_code(db, s);
        GHashTable *tab = tagdb_get_item(db, n, TAG_TABLE); // may return NULL
        //log_hash(tab);
        if (c == '=')
        {
            char *rhs = tokenizer_next(tok, &c);
            int type = tagdb_get_tag_type(db, s);
            union tagdb_value *val = tagdb_str_to_value(type, rhs);
            gpointer data[] = {TO_P(type), val};
            tab = set_subset(tab, value_equals, (gpointer) data);
        }
        if (op == '/') // intersection
        {
            r = set_intersect_s(r, tab);
        }
        else if (op == '\\') // union
        {
            r = set_union_s(r, tab);
        }
        else if (op == '~') // rel comp
        {
            r = set_difference_s(r, tab);
        }
        g_free(s);
        op = c;
        s = tokenizer_next(tok, &c);
    }
    tokenizer_destroy(tok);
    // pack with the tag information
    GHashTable *res = g_hash_table_new(g_direct_hash, g_direct_equal);
    GHashTableIter it;
    gpointer k, v;
    g_hash_loop(r, it, k, v)
    {
        g_hash_table_insert(res, k, tagdb_get_item(db, GPOINTER_TO_INT(k), FILE_TABLE));
    }

    *result = res;
    *type = tagdb_dict_t;
}
q_fn q_functions[2][4] = {// Tag table funcs
    {
        tagdb_file_remove,
        tagdb_file_has_tags,
        tagdb_file_create,
        tagdb_file_add_tags
    },
    {
        tagdb_tag_is_empty,
        tagdb_tag_remove,
        tagdb_tag_tspec,
        tagdb_tag_create
    }
};

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
    char *token = NULL;
    GList *seps = g_list_new_charlist(' ', NULL);
    Tokenizer *tok = tokenizer_new(seps);
    /*
    struct mallinfo minf = mallinfo();
    log_msg("used space = %d\n", minf.uordblks);
    log_msg("free space = %d\n", minf.fordblks);
    log_msg("total space = %d\n", minf.arena);
    */
    tokenizer_set_str_stream(tok, qs);
    g_free(qs);
    token = tokenizer_next(tok, &sep);
    if (g_strcmp0(token, "FILE") == 0)
    {
        qr->table_id = 0;
    }
    else if (g_strcmp0(token, "TAG") == 0)
    {
        qr->table_id = 1;
    }
    else // malformatted
    {
        return NULL;
    }
    g_free(token);
    token = tokenizer_next(tok, &sep);
    qr->command_id = _name_to_code(token, q_commands[qr->table_id]);
    g_free(token);
    token = tokenizer_next(tok, &sep);
    int i = 0;
    while (token != NULL && i < 10)
    {
        qr->argv[i] = token;
        i++;
        token = tokenizer_next(tok, &sep);
    }
    tokenizer_destroy(tok);
    qr->argv[i] = NULL;
    qr->argc = i;
    //log_msg("PARSING\n");
    return qr;
}

// action takes a compiled query and performs the action on the database db
// query
void act (tagdb *db, query_t *q, gpointer *result, int *type)
{
    //log_query_info(q);
    q_functions[q->table_id][q->command_id](db, q->table_id, q->argc, q->argv, result, type);
}
// -> (type, *object*)
// encapsualtes the object in a result type
// with info about the type based on type
// we pass the database because encapsulation may make further
// queries to acquire data that the user expects
result_t *encapsulate (int type, gpointer data)
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
        default:
            res->data.b = data;
    }
    return res;
}

void query_destroy (query_t *q)
{
    //g_strfreev(q->argv);
    g_free(q);
}

void result_destroy (result_t *r)
{
    switch (r->type)
    {
        case tagdb_int_t:
            break;
        case tagdb_str_t:
            g_free(r->data.s);
            break;
        case tagdb_list_t:
            g_list_free(r->data.l);
            break;
        case tagdb_dict_t:
            g_hash_table_unref(r->data.d);
            break;
        default:
            g_free(r->data.b);
    }
    g_free(r);
}

void query_info (query_t *q)
{
    if (q==NULL)
    {
        fprintf(stderr, "query_info: got q==NULL\n");
        return;
    }
    printf("query info:\n");
    printf("\ttable_id: %s\n", (q->table_id==FILE_TABLE)?"FILE_TABLE":"TAG_TABLE");
    printf("\tcommand: %s\n", q_commands[q->table_id][q->command_id]);
    printf("\targc: %d\n", q->argc);
    int i;
    for (i = 0; i < q->argc; i++)
    {
        printf("\targv[%d] = %s\n", i, q->argv[i]);
    }
}
