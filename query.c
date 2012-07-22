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
enum query_classes
{
    FILE_QUERY,
    TAG_QUERY
};

#define WIP 0

const char *q_commands[][8] = {
    // File table commands
    {
#if WIP
        // {Name, Return type}
        "REMOVE",
        "CREATE",
        "ADD_TAGS",
        "RENAME",
        "HAS_TAGS",
        "LIST_TAGS",
#endif
        "SEARCH",
        NULL
    },
    // Tag table commands
    {
#if WIP
        "REMOVE",
        "CREATE",
        "ADD_TAGS",
        "RENAME",
        "IS_EMPTY",
#endif
        "SEARCH",
        NULL
    },
};

/* The operators used in the "TAG SEARCH" query */
const char *search_operators[2][4] = {
    {"/", "\\", "%", NULL},
    {"AND", "OR", "ANDN", NULL},
};

set_operation_l oper_fn[] = {
    g_list_intersection, g_list_union, g_list_difference
};


/* limiters for use in SEARCH */
const char *search_limiters[] = {"<", "=", ">", NULL};
static set_predicate lim_pred_functions[] = {
    value_lt_sp, value_eq_sp, value_gt_sp
};

const char *special_tags[3] = {"@all", "@untagged", NULL};
special_tag_fn special_tag_functions[2] = {
    tagdb_all_files, tagdb_untagged_items
};


// checks if the argc is correct and fills the result and type with the appropriate error
int check_argc (int argc, int required, gpointer *result, char **type)
{
    if (argc < required)
    {
        *type = "E";
        *result = g_strdup("too few arguments"); // TODO: make a real error object
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

#if WIP
void tagdb_tag_is_empty (TagDB *db, int argc, gchar **argv, gpointer *result, char **type)
{
    check_args(1);
    Tag *t = lookup_tag(db, arg[0]);
    if (!t)
    {
        *type = tagdb_err_t;
        *result = g_strdup("invalid tag name");
        return;
    }
    *result = GINT_TO_POINTER(file_slot_size(db, t->id)?FALSE:TRUE);
    *type = tagdb_int_t;
}

void tagdb_file_has_tags (TagDB *db, int argc, gchar **argv, gpointer *result, char **type)
{
    if (!check_argc(argc, 1, result, type))
        return;
    GHashTable *tmp = tagdb_get_item(db, atoi((char*) argv[0]), class_id);
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
#endif

void tagdb_tag_rename (TagDB *db, int argc, gchar **argv, gpointer *result, char **type)
{
    check_args(2);
    Tag *t = lookup_tag(db, argv[0]);
    set_tag_name(t, argv[1], db);
    *type = "I";
    *result = TO_P(TRUE);
}

#if WIP
void tagdb_tag_remove (TagDB *db, int argc, gchar **argv, gpointer *result, char **type)
{
    check_args(1);
    tagdb_remove_item(db, tagdb_get_tag_code(db, argv[0]), class_id);
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

void tagdb_x_add_tags (TagDB *db, int argc, gchar **argv, gpointer *result, char **type)
{
    check_args(2);

    // we shouldn't create a file
    int item_id = atoi(argv[0]);
    if (tagdb_get_item(db, item_id, class_id) == NULL)
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

                tagdb_insert_sub(db, item_id, tagcode, value, class_id);
                g_free(tag_val);
            }
        }
        g_free(tag_name);
    }
    *type = tagdb_int_t;
    *result = TO_P(item_id);
}

void tagdb_tag_add_tags (TagDB *db, int argc, gchar **argv, gpointer *result, char **type)
{
    check_args(2);
    int tcode = tagdb_get_tag_code(db, argv[0]);

    ID_TO_STRING(id_string,tcode);

    argv[0] = id_string;
    tagdb_x_add_tags(db,META_TABLE,argc,argv,result,type);
    *result = tcode;
    *type = tagdb_int_t;
}

void tagdb_tag_create (TagDB *db, int argc, gchar **argv, gpointer *result, char **type)
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

    tcode = tagdb_insert_item(db, argv[0], NULL, class_id);

    tagdb_set_tag_type(db, argv[0], our_type);

    ID_TO_STRING(id_string, tcode);

    argc-- ;
    char **new_argv = make_new_argv(1, argc - 1, argv + 2 , id_string);

    int i;
    for (i = 0 ; i < argc; i++)
        log_msg("%s\n", new_argv[i]);
    tagdb_x_add_tags(db, class_id, argc, new_argv, result, type);

    g_free(new_argv);
    *type = tagdb_int_t;
    *result = TO_P(tcode);
}

void tagdb_file_rename (TagDB *db, int argc, gchar **argv, gpointer *result, char **type)
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
void tagdb_file_list_tags (TagDB *db, int argc, gchar **argv, gpointer *result, char **type)
{
    check_args(1);
    int file_id = atoi(argv[0]);
    *result = tagdb_get_item(db, file_id, FILE_QUERY);
    *type = tagdb_dict_t;
}

/*
   Arguments:
       - (tag:value)*::(STRING:%tag_type%)*
   Return:
       The id of the file created. :: INT
 */
void tagdb_file_create (TagDB *db, int argc, gchar **argv, gpointer *result, char **type)
{
    if (argc == 0)
    {
        *type = tagdb_int_t;
        *result = TO_P(tagdb_insert_item(db, NULL, NULL, FILE_QUERY));
        return;
    }

    int file_id = tagdb_insert_item(db, NULL, NULL, FILE_QUERY);

    ID_TO_STRING(id_string, file_id);

    char **new_argv = make_new_argv(1, argc, argv, id_string);

    tagdb_x_add_tags(db, argc + 1, new_argv, result, type);
    //g_free(new_argv);
}
#endif

// predicate for equality in search
gboolean value_eq_sp (gpointer key, gpointer lvalue, gpointer value)
{
    return tagdb_value_equals((tagdb_value_t*) lvalue, (tagdb_value_t*) value);
}

gboolean value_lt_sp (gpointer key, gpointer lvalue, gpointer value)
{
    return (tagdb_value_cmp((tagdb_value_t*) lvalue, (tagdb_value_t*) value) < 0);
}

gboolean value_gt_sp (gpointer key, gpointer lvalue, gpointer value)
{
    return (tagdb_value_cmp((tagdb_value_t*) lvalue, (tagdb_value_t*) value) > 0);
}
// returns a hash table for the given tag
GList *_get_tag_table (TagDB *db, char *tag_name)
{
    int idx = strv_index(special_tags, tag_name);

//    log_msg("Entering _get_tag_table\n");
//    log_msg("_get_tag_table tag_name = \"%s\"\n", tag_name);
//    log_msg("_get_tag_table idx=%d\n", idx);

    if (idx != -1)
    {
        return special_tag_functions[idx](db);
    }
    char *tag;
    char *sep;

    Scanner *scn = scanner_new_v(search_limiters);
    scanner_set_quotes(scn, g_list_new("\"", NULL));
    scanner_set_str_stream(scn, tag_name);

    tag = scanner_next(scn, &sep);
    Tag *t = lookup_tag(db, tag);

    GList *res = NULL;

    if (t)
    {
        res = file_cabinet_get_drawer_l(db->files, t->id);
        idx = strv_index(search_limiters, sep);

        if (!scanner_stream_is_empty(scn->stream))
        {
            int type = t->type;

            char *vstring = scanner_next(scn, &sep);
            tagdb_value_t *val = tagdb_str_to_value(type, vstring);
            GList *tmp = g_list_filter(res, lim_pred_functions[idx], val);
            g_list_free(res);
            res = tmp;
            result_destroy(val);
        }
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
void tagdb_file_search (TagDB *db, int argc, gchar **argv, gpointer *result, char **type)
{
    check_args(1);
    if (argc % 2 != 1)
    {
        *result = g_strdup("invalid query");
        *type = "E";
        return;
    }

    int i;
    int op_idx = -1;

    GList *files = NULL;
    for (i = 0; i < argc; i += 2 )
    {
        GList *this_table = _get_tag_table(db, argv[i]);
        GList *tmp = NULL;

        if (op_idx != -1)
            tmp = oper_fn[op_idx](files, this_table, (GCompareFunc) file_id_cmp);
        else
            tmp = g_list_copy(this_table);

        g_list_free(files);
        g_list_free(this_table);
        files = tmp;

        op_idx = strv_index(search_operators[1], argv[i+1]);
    }

    GList *res = NULL;
    LL(files, it)
    {
        File *f = it->data;
        GList *pair = NULL;
        GList *tags = NULL;
        HL(f->tags, it, k, v)
        {
            GList *pair = NULL;
            Tag *t = retrieve_tag(db, TO_S(k));
            char *s = tagdb_value_to_str(v);
            pair = g_list_new(g_strdup(t->name), s, NULL);
            tags = g_list_prepend(tags, pair);
        } HL_END;
        tags = g_list_prepend(tags,
                g_list_new(g_strdup("name"), g_strdup(f->name)));
        pair = g_list_new(TO_SP(f->id), tags, NULL);
        res = g_list_prepend(res, pair);
    } LL_END;

    g_list_free(files);

    /* {file_name => {tag_name => tag_value}} */
    *type = "DIDSS";
    *result = res;
}

q_fn q_functions[][7] = {// Tag table funcs
    {
#if WIP
        tagdb_file_remove,
        tagdb_file_create,
        tagdb_x_add_tags,
        tagdb_file_rename,
        tagdb_file_has_tags,
        tagdb_file_list_tags,
#endif
        tagdb_file_search,
    },
    {
#if WIP
        tagdb_tag_remove,
        tagdb_tag_create,
        tagdb_tag_add_tags,
        tagdb_tag_rename,
        tagdb_tag_is_empty,
#endif
        tagdb_file_search
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
    qr->command_id = strv_index(q_commands[qr->class_id], token);
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
    q_functions[q->class_id][q->command_id](db, q->argc, q->argv, result, type);
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
    p("\tclass_id: %s\n", (q->class_id==FILE_QUERY)?"FILE_QUERY":(
                               (q->class_id==TAG_QUERY)?"TAG_QUERY":"META_TABLE"));
    p("\tcommand: %s\n", q_commands[q->class_id][q->command_id]);
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
    char *type = "E";
    act(db, q, &r, &type);
    query_destroy(q);
    result_t *res = encapsulate(type, r);
    res_info(res, log_msg0);
    return res;
}
