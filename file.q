#include <glib.h>
#include "scanner.h"
#include "tagdb.h"
#include "query.h"
#include "query_file.h"

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
q%search 1 DIDSS%
{
    if (argc % 2 != 1)
    {
        qerr%invalid query testing%
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
    qr%res%
}

q%list_tags 1 DSS%
{
    File *f = retrieve_file(db, atoll(argv[0]));
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
    qr%tags%
}
