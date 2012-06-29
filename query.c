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

static int _log_level = 1;

#define check_args(num) \
    if (!check_argc(argc, num, result, type)) \
        return;

const char *q_commands[][8] = {
    // File table commands
    {
        "REMOVE",
        "CREATE",
        "ADD_TAGS",
        "RENAME",
        "HAS_TAGS",
        "LIST_TAGS",
        "SEARCH",
        NULL
    },
    // Tag table commands
    {
        "REMOVE",
        "CREATE",
        "ADD_TAGS",
        "RENAME",
        "IS_EMPTY",
        "SEARCH",
        NULL
    },
};

/* The operators used in the "TAG SEARCH" query */
const char *search_operators[2][4] = {
    {"/", "\\", "%", NULL},
    {"AND", "OR", "ANDN", NULL},
};

set_operation oper_fn[] = {
    set_intersect_s, set_union_s, set_difference_s
};


/* limiters for use in SEARCH */
const char *search_limiters[] = {"<", "=", ">", NULL};
static set_predicate lim_pred_functions[] = {
    value_lt_sp, value_eq_sp, value_gt_sp
};

const char *special_tags[4] = {"@all", "@tagged", "@untagged", NULL};
special_tag_fn special_tag_functions[3] = {
    tagdb_get_table, tagdb_tagged_items, tagdb_untagged_items
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

/* Query functions
   
   Functions must return the type of their result (as enumerated in types.h) and the result itself
   the function returns these in the two result arguments it is given
   functions must accept an argument count and an argument list (NULL-terminated array of strings)
   finally, functions will take a pointer to a TagDB and a table id as named arguments,
   thus functions will have the form:
     void (*func) (TagDB *db, int argc, char **argv, gpointer result, int type) */

void tagdb_tag_is_empty (TagDB *db, int argc, gchar **argv, gpointer *result, int *type)
{
    check_args(1);
    Tag *t = lookup_tag(db, arg[0]);
    if (!t)
    {
        *type = tagdb_err_t;
        *result = "invalid tag name";
        return;
    }
    *result = GINT_TO_POINTER(file_slot_size(db, t->id)?FALSE:TRUE);
    *type = tagdb_int_t;
}

void tagdb_file_has_tags (TagDB *db, int argc, gchar **argv, gpointer *result, int *type)
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

void tagdb_tag_rename (TagDB *db, int argc, gchar **argv, gpointer *result, int *type)
{
    check_args(2);
    Tag *t = lookup_tag(db, argv[0]);
    set_tag_name(t, argv[1]);
    *type = tagdb_int_t;
    *result = TO_P(TRUE);
}

void tagdb_tag_remove (TagDB *db, int argc, gchar **argv, gpointer *result, int *type)
{
    check_args(1);
    tagdb_remove_item(db, tagdb_get_tag_code(db, argv[0]), table_id);
}

// Makes a new argument vector with new_args followed by
// old_args_num arguments from old_argv 
char **make_new_argv (int new_args_num, int old_args_num, char **old_argv, ... /* new_args */)
{
    int argc = new_args_num + old_args_num;

    va_list new_args;
    va_start(new_args, old_argv);
    char **new_argv = calloc(argc+1, sizeof(char*));
    
    int i;
    for (i = 0 ; i < new_args_num; i++)
        new_argv[i] = va_arg(new_args, char*);

    for (i = 0; i < old_args_num; i++)
        new_argv[i+new_args_num] = old_argv[i];
    
    new_argv[argc] = NULL;
    return new_argv;
}

void tagdb_x_add_tags (TagDB *db, int argc, gchar **argv, gpointer *result, int *type)
{
    check_args(2);
    
    // we shouldn't create a file
    int item_id = atoi(argv[0]);
    if (tagdb_get_item(db, item_id, table_id) == NULL)
    {
        *result = TO_P(0);
        *type = tagdb_int_t;
        return;
    }
    argc--;
    argv++;

    char *sep = NULL;
    const char *seps[] = {":", NULL};
    const char *quots[] = {"\"", NULL};
    Scanner *scn = scanner_new2_v(seps, quots);

    int i;
    
    for (i = 0; i < argc; i++)
    {
        _log_level = 0;
        //printf("argv[%d] = %s", i, argv[i]);
        scanner_set_str_stream(scn, argv[i]);
        char *tag_name = scanner_next(scn, &sep);
        int tagcode = tagdb_get_tag_code(db, tag_name);
        if (tagcode > 0)
        {
            int tagtype = tagdb_get_tag_type_from_code(db, tagcode);
            if (g_strcmp0(sep, ":") == 0)
            {
                char *tag_val = scanner_next(scn, &sep);
                
                tagdb_value_t *value = NULL;

                if (strlen(tag_val))
                    value = tagdb_str_to_value(tagtype, tag_val);

                tagdb_insert_sub(db, item_id, tagcode, value, table_id);
                g_free(tag_val);
            }
        }
        g_free(tag_name);
    }
    *type = tagdb_int_t;
    *result = TO_P(item_id);
}

void tagdb_tag_add_tags (TagDB *db, int argc, gchar **argv, gpointer *result, int *type)
{
    check_args(2);
    int tcode = tagdb_get_tag_code(db, argv[0]);

    ID_TO_STRING(id_string,tcode);
    
    argv[0] = id_string;
    tagdb_x_add_tags(db,META_TABLE,argc,argv,result,type);
    *result = tcode;
    *type = tagdb_int_t;
}

void tagdb_tag_create (TagDB *db, int argc, gchar **argv, gpointer *result, int *type)
{
    _log_level = 0;
    check_args(2);

    extern const char *type_strings[];

    int our_type = strv_index(type_strings, argv[1]);
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

    tcode = tagdb_insert_item(db, argv[0], NULL, table_id);

    tagdb_set_tag_type(db, argv[0], our_type);

    ID_TO_STRING(id_string, tcode);

    argc-- ; 
    char **new_argv = make_new_argv(1, argc - 1, argv + 2 , id_string);

    int i;
    for (i = 0 ; i < argc; i++)
        log_msg("%s\n", new_argv[i]);
    tagdb_x_add_tags(db, table_id, argc, new_argv, result, type);

    g_free(new_argv);
    *type = tagdb_int_t;
    *result = TO_P(tcode);
}

void tagdb_file_rename (TagDB *db, int argc, gchar **argv, gpointer *result, int *type)
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
void tagdb_file_list_tags (TagDB *db, int argc, gchar **argv, gpointer *result, int *type)
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
void tagdb_file_create (TagDB *db, int argc, gchar **argv, gpointer *result, int *type)
{
    if (argc == 0)
    {
        *type = tagdb_int_t;
        *result = TO_P(tagdb_insert_item(db, NULL, NULL, FILE_TABLE));
        return;
    }

    int file_id = tagdb_insert_item(db, NULL, NULL, FILE_TABLE);

    ID_TO_STRING(id_string, file_id);

    char **new_argv = make_new_argv(1, argc, argv, id_string);

    tagdb_x_add_tags(db, table_id, argc + 1, new_argv, result, type);
    //g_free(new_argv);
}

// predicate for equality in search
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
GHashTable *_get_tag_table (TagDB *db, char *tag_name)
{
//    log_msg("Entering _get_tag_table\n");
//    log_msg("_get_tag_table tag_name = \"%s\"\n", tag_name);
    int idx = strv_index(special_tags, tag_name);
//    log_msg("_get_tag_table idx=%d\n", idx);
    if (idx != -1)
    {
        return special_tag_functions[idx](db, table_id);
    }
    char *tag;
    char *sep;

    Scanner *scn = scanner_new_v(search_limiters);
    scanner_set_quotes(scn, g_list_new("\"", NULL));
    scanner_set_str_stream(scn, tag_name);

    tag = scanner_next(scn, &sep);

    int item_id;
    if (table_id == TAG_TABLE)
        item_id = atoi(tag_name); // our tag is a file id
    else
        item_id = tagdb_get_tag_code(db, tag); // our tag is a tag
    int other = get_other_table_id(table_id);
    _log_level = 0;
    log_msg("this = %d\nother = %d\n", table_id, other);
    GHashTable *res = tagdb_get_item(db, item_id, other);
    if (res == NULL)
        return NULL;
    idx = strv_index(search_limiters, sep);
//    log_msg("_get_tag_table, strv_index(search_limiters, %s) idx=%d\n", sep, idx);

    if (!scanner_stream_is_empty(scn->stream))
    {
        int type = tagdb_get_tag_type(db, tag);

        char *vstring = scanner_next(scn, &sep);
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
void tagdb_x_search (TagDB *db, int argc, gchar **argv, gpointer *result, int *type)
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
        GHashTable *this_table = _get_tag_table(db, table_id, argv[i]);
        if (this_table == NULL)
        {
            *result = "invalid query";
            *type = tagdb_err_t;
            return;
        }
        if (op_idx != -1)
            tab = oper_fn[op_idx](tab, this_table); 
        else
            tab = this_table;
        op_idx = strv_index(search_operators[1], argv[i+1]);
    }
    // pack with the tag information
    GHashTable *res = g_hash_table_new(g_direct_hash, g_direct_equal);
    if (tab != NULL)
    {
        GHashTableIter it;
        gpointer k, v;
        g_hash_loop(tab, it, k, v)
        {
            g_hash_table_insert(res, k, tagdb_get_item(db, GPOINTER_TO_INT(k), table_id));
        }
    }
    *result = res;
    *type = tagdb_dict_t;
}

q_fn q_functions[][7] = {// Tag table funcs
    {
        tagdb_file_remove,
        tagdb_file_create,
        tagdb_x_add_tags,
        tagdb_file_rename,
        tagdb_file_has_tags,
        tagdb_file_list_tags,
        tagdb_x_search,
    },
    {
        tagdb_tag_remove,
        tagdb_tag_create,
        tagdb_tag_add_tags,
        tagdb_tag_rename,
        tagdb_tag_is_empty,
        tagdb_x_search
    },
    {
        tagdb_x_search,
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
    Scanner *scn = scanner_new2(seps, quotes);
    scanner_set_str_stream(scn, qs);
    g_free(qs);
    token = scanner_next(scn, &sep);
    const char *table_strings[] = {"FILE", "TAG", "META", NULL};
    qr->table_id = strv_index(table_strings, token);
    if (qr->table_id == -1)
    {
        free(qr);
        return NULL;
    }
    g_free(token);

    token = scanner_next(scn, &sep);
    qr->command_id = strv_index(q_commands[qr->table_id], token);
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
    //log_msg("PARSING\n");
//    log_msg("Exiting parse\n");
    return qr;
}

// action takes a compiled query and performs the action on the database db
// query
void act (TagDB *db, query_t *q, gpointer *result, int *type)
{
    log_query_info(q);
    q_functions[q->table_id][q->command_id](db, q->table_id, q->argc, q->argv, result, type);
//    log_msg("Exiting act\n");
}

void log_query_info (query_t *q)
{
    lock_log();
    if (q==NULL)
    {
        log_msg0("query_info: got q==NULL\n");
        return;
    }
    log_msg0("query info:\n");
    log_msg0("\ttable_id: %s\n", (q->table_id==FILE_TABLE)?"FILE_TABLE":(
                               (q->table_id==TAG_TABLE)?"TAG_TABLE":"META_TABLE"));
    log_msg0("\tcommand: %s\n", q_commands[q->table_id][q->command_id]);
    log_msg0("\targc: %d\n", q->argc);
    int i;
    for (i = 0; i < q->argc; i++)
    {
        log_msg0("\targv[%d] = %s\n", i, q->argv[i]);
    }
    unlock_log();
}

void query_info (query_t *q)
{
    if (q==NULL)
    {
        fprintf(stderr, "query_info: got q==NULL\n");
        return;
    }
    printf("query info:\n");
    printf("\ttable_id: %s\n", (q->table_id==FILE_TABLE)?"FILE_TABLE":(
                               (q->table_id==TAG_TABLE)?"TAG_TABLE":"META_TABLE"));
    printf("\tcommand: %s\n", q_commands[q->table_id][q->command_id]);
    printf("\targc: %d\n", q->argc);
    int i;
    for (i = 0; i < q->argc; i++)
    {
        printf("\targv[%d] = %s\n", i, q->argv[i]);
    }
}

// does all of the steps for you
result_t *tagdb_query (TagDB *db, const char *query)
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
