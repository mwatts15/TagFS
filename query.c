/* Query API for tagdb
 *
 * Parses query strings and calls the appropriate methods from tagdb
 * also handles conversions from the external strings to internal ids
 * when necessary
 */
#include <malloc.h>
#include <string.h>
#include "query.h"
#include "types.h"
#include "tokenizer.h"
#include "set_ops.h"
#include "util.h"
#include "log.h"

static int _log_level = 1;

#define check_args(num) \
    if (!check_argc(argc, num, result, type)) \
        return;

const char *q_commands[2][7] = {
    // File table commands
    {
        "REMOVE",
        "HAS_TAGS",
        "CREATE",
        "ADD_TAGS",
        "RENAME",
        "LIST_TAGS",
        NULL
        // Other commands
    },
    // Tag table commands
    {
        "IS_EMPTY",
        "REMOVE",
        "TSPEC",
        "CREATE",
        "RENAME",
        NULL
    },
};

/* The operators used in the "TAG TSPEC" query */
const char *tspec_operators[2][4] = {
    {"/", "\\", "%", NULL},
    {"AND", "OR", "ANDN", NULL},
};

set_operation oper_fn[] = {
    set_intersect_s, set_union_s, set_difference_s
};


/* limiters for use in TSPEC */
const char *tspec_limiters[] = {"<", "=", ">", NULL};
static set_predicate lim_pred_functions[] = {
    value_lt_sp, value_eq_sp, value_gt_sp
};

const char *special_tags[4] = {"@all", "@tagged", "@untagged", NULL};
special_tag_fn special_tag_functions[3] = {
    tagdb_files, tagdb_tagged_files, tagdb_untagged_files
};


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
        if (str_equal(table[i], name))
            return i;
        i++;
    }
    return -1;
}

// Functions must return the type of their result (as enumerated in types.h) and the result itself
// the function returns these in the two result arguments it is given
// functions must accept an argument count and an argument list (NULL-terminated array of strings)
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

void tagdb_tag_rename (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type)
{
    if (!check_argc(argc, 2, result, type))
        return;
    tagdb_change_tag_name(db, argv[0], argv[1]);
    *type = tagdb_int_t;
    *result = TO_P(TRUE);
}

void tagdb_tag_remove (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type)
{
    if (!check_argc(argc, 1, result, type))
        return;
    tagdb_remove_item(db, tagdb_get_tag_code(db, argv[0]), table_id);
}

void tagdb_tag_create (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type)
{
    check_args(2);

    extern const char *type_strings[];

    int our_type = _name_to_code(argv[1], type_strings);
    if (our_type == tagdb_err_t)
    {
        *type = tagdb_err_t;
        *result = "invalid type";
        return;
    }
    int tcode = tagdb_get_tag_code(db, argv[0]);
    if (tcode > 0)
    {
        *type = tagdb_err_t;
        *result = "tag exists";
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

    char *sep = NULL;
    const char *seps[] = {":", NULL};
    const char *quots[] = {"\"", NULL};
    Tokenizer *tok = tokenizer_new2_v(seps, quots);

    int i;
    
    for (i = 0; i < argc; i++)
    {
        _log_level = 0;
        log_msg("argv[%d] = %s", i, argv[i]);
        tokenizer_set_str_stream(tok, argv[i]);
        char *tag_name = tokenizer_next(tok, &sep);
        int tagcode = tagdb_get_tag_code(db, tag_name);
        if (tagcode > 0)
        {
            int tagtype = tagdb_get_tag_type_from_code(db, tagcode);
            if (g_strcmp0(sep, ":") == 0)
            {
                char *tag_val = tokenizer_next(tok, &sep);
                
                tagdb_value_t *value = NULL;

                if (strlen(tag_val))
                    value = tagdb_str_to_value(tagtype, tag_val);

                tagdb_insert_sub(db, file_id, tagcode, value, FILE_TABLE);
                g_free(tag_val);
            }
        }
        g_free(tag_name);
    }
    *type = tagdb_int_t;
    *result = (gpointer) file_id;
}

void tagdb_file_rename (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type)
{
    if (!check_argc(argc, 2, result, type))
        return;
    // argv[0] = file id
    // argv[1] = new name
    int file_id = atoi(argv[0]);
    char *str = g_strdup_printf("FILE ADD_TAGS %d name:\"%s\"", file_id, argv[1]);
    result_t *res = tagdb_query(db, str);
    *type = res->type;
    *result = TO_P(res->data.i); // If it is an error this should be okay
}

/*
   Arguments:
       - file_id::INT
    Return:
        A hash table of the tags with their values :: HASH
 */
void tagdb_file_list_tags (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type)
{
    check_args(1);
    int file_id = atoi(argv[0]);
    *result = tagdb_get_item(db, file_id, FILE_TABLE);
    *type = tagdb_dict_t;
}
/*
   Arguments:
       - (tag:value)*::(STRING:%tag_type%)*
   Return: 
       The id of the file created. :: INT
 */
void tagdb_file_create (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type)
{
    if (argc == 0)
    {
        *type = tagdb_int_t;
        *result = TO_P(tagdb_insert_item(db, NULL, NULL, FILE_TABLE));
        return;
    }

    int file_id = tagdb_insert_item(db, NULL, NULL, FILE_TABLE);
    argc++;

    char **new_argv = calloc(argc+1, sizeof(char*));
    int maxlen = 16;
    char id_string[maxlen];
    g_snprintf(id_string, maxlen, "%d", file_id);
    new_argv[0] = id_string;
    int i;
    for (i = 0 ; i < argc; i++)
        new_argv[i+1] = argv[i];
    new_argv[argc] = NULL;
    tagdb_file_add_tags(db, table_id, argc, new_argv, result, type);
}

// predicate for equality in tspec
gboolean value_eq_sp (gpointer key, gpointer value, gpointer lvalue)
{
    return tagdb_value_equals((tagdb_value_t*) lvalue, (tagdb_value_t*) value);
}

gboolean value_lt_sp (gpointer key, gpointer value, gpointer lvalue)
{
    return (tagdb_value_cmp((tagdb_value_t*) lvalue, (tagdb_value_t*) value) < 0);
}

gboolean value_gt_sp (gpointer key, gpointer value, gpointer lvalue)
{
    return (tagdb_value_cmp((tagdb_value_t*) lvalue, (tagdb_value_t*) value) > 0);
}
// returns a hash table for the given tag
GHashTable *_get_tag_table (tagdb *db, char *tag_name)
{
//    log_msg("Entering _get_tag_table\n");
//    log_msg("_get_tag_table tag_name = \"%s\"\n", tag_name);
    int idx = strv_index(special_tags, tag_name);
//    log_msg("_get_tag_table idx=%d\n", idx);
    if (idx != -1)
    {
        return special_tag_functions[idx](db);
    }
    char *tag;
    char *sep;

    Tokenizer *tok = tokenizer_new_v(tspec_limiters);
    tokenizer_set_quotes(tok, g_list_new("\"", NULL));
    tokenizer_set_str_stream(tok, tag_name);

    tag = tokenizer_next(tok, &sep);
    int tcode = tagdb_get_tag_code(db, tag);

    GHashTable *res = tagdb_get_item(db, tcode, TAG_TABLE);
    if (res == NULL)
        return NULL;
    idx = strv_index(tspec_limiters, sep);
//    log_msg("_get_tag_table, strv_index(tspec_limiters, %s) idx=%d\n", sep, idx);

    if (!tokenizer_stream_is_empty(tok->stream))
    {
        int type = tagdb_get_tag_type(db, tag);

        char *vstring = tokenizer_next(tok, &sep);
        tagdb_value_t *val = tagdb_str_to_value(type, vstring);
        return set_subset(res, lim_pred_functions[idx], val);
    }

    return res;
}

/*
   Arguments:
       - (tag, operator)* :: (STRING, %operator)*
       - last_tag :: STRING
   Return: 
       A dict of ids matching the query or NULL :: DICT
 */
void tagdb_tag_tspec (tagdb *db, int table_id, int argc, gchar **argv, gpointer *result, int *type)
{
    check_args(1);
    if (argc % 2 != 1)
    {
        *result = "invalid query";
        *type = tagdb_err_t;
        return;
    }

    int i;
    int op_idx = -1;
    GHashTable *tab = NULL;
    for (i = 0; i < argc; i += 2 )
    {
        GHashTable *this_table = _get_tag_table(db, argv[i]);
        if (this_table == NULL)
        {
            *result = NULL;
            *type = tagdb_dict_t;
            return;
        }
        if (op_idx != -1)
            tab = oper_fn[op_idx](tab, this_table); 
        else
            tab = this_table;
        op_idx = strv_index(tspec_operators[1], argv[i+1]);
    }
    // pack with the tag information
    GHashTable *res = g_hash_table_new(g_direct_hash, g_direct_equal);
    if (tab != NULL)
    {
        GHashTableIter it;
        gpointer k, v;
        g_hash_loop(tab, it, k, v)
        {
            g_hash_table_insert(res, k, tagdb_get_item(db, GPOINTER_TO_INT(k), FILE_TABLE));
        }
    }
    *result = res;
    *type = tagdb_dict_t;
}

q_fn q_functions[2][6] = {// Tag table funcs
    {
        tagdb_file_remove,
        tagdb_file_has_tags,
        tagdb_file_create,
        tagdb_file_add_tags,
        tagdb_file_rename,
        tagdb_file_list_tags
    },
    {
        tagdb_tag_is_empty,
        tagdb_tag_remove,
        tagdb_tag_tspec,
        tagdb_tag_create,
        tagdb_tag_rename,
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
    log_msg("");
    if (s == NULL)
        return NULL;
    query_t *qr = malloc(sizeof(query_t));
    char *qs = g_strstrip(g_strdup(s));
    char *sep;
    char *token = NULL;
    GList *seps = g_list_new(" ", NULL);
    GList *quotes = g_list_new("\"", NULL);
    Tokenizer *tok = tokenizer_new2(seps, quotes);
    tokenizer_set_str_stream(tok, qs);
    g_free(qs);
    token = tokenizer_next(tok, &sep);
    if (str_equal(token, "FILE"))
    {
        qr->table_id = 0;
    }
    else if (str_equal(token, "TAG"))
    {
        qr->table_id = 1;
    }
    else // malformatted
    {
        free(qr);
        return NULL;
    }
    g_free(token);

    token = tokenizer_next(tok, &sep);
    qr->command_id = _name_to_code(token, q_commands[qr->table_id]);
    g_free(token);
    if (qr->command_id == -1)
    {
        free(qr);
        return NULL;
    }
    int i = 0;
    while (!tokenizer_stream_is_empty(tok->stream) && i < MAX_QUERY_ARGS)
    {
        token = tokenizer_next(tok, &sep);
        qr->argv[i] = token;
        i++;
    }
    tokenizer_destroy(tok);
    qr->argv[i] = NULL;
    qr->argc = i;
    //log_msg("PARSING\n");
//    log_msg("Exiting parse\n");
    return qr;
}

// action takes a compiled query and performs the action on the database db
// query
void act (tagdb *db, query_t *q, gpointer *result, int *type)
{
    log_query_info(q);
    q_functions[q->table_id][q->command_id](db, q->table_id, q->argc, q->argv, result, type);
//    log_msg("Exiting act\n");
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

// does all of the steps for you
result_t *tagdb_query (tagdb *db, const char *query)
{
    gpointer r = NULL;
    query_t *q = parse(query);
    if (q == NULL)
        return NULL;
    int type = -1;
    act(db, q, &r, &type);
    query_destroy(q);
    return encapsulate(type, r);
}
