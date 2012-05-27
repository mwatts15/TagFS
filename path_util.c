/* utilities for manipulating paths */
#include <glib.h>
#include <libgen.h>
#include "query.h"
#include "util.h"
#include "types.h"
#include "tokenizer.h"
#include "tagfs.h"
#include "params.h"

// returns the file in our copies directory corresponding to
// the one in path
// should only be called on regular files since
// directories are only virtual
char *tagfs_realpath(const char *path)
{
    char *res = g_strconcat(TAGFS_DATA->copiesdir, "/", path, NULL);
    //log_msg("tagfs_realpath(path=\"%s\") = \"%s\"\n", path, res);
    return res;
}

char *_op_sym_to_name (const char *sym)
{
    return tspec_operators[1][strv_index(tspec_operators[0], sym)];
}

char *translate_path (const char *path)
{
    // translates the path
    //   /path/to\some=random%tag
    // to
    //   AND "path" AND "to" OR "some=random" ANDN "tag"
    // the format expected by tspec
    if (g_strcmp0("/", path) == 0)
        return g_strdup("@all");
    char *str = g_strconcat("@all", path, NULL);
    Tokenizer *tok = tokenizer_new_v(tspec_operators[0]);

    tokenizer_set_str_stream(tok, str);
    g_free(str);

    GString *accu = g_string_new("");
    char *token;
    char *sep;
    char *op;

    while(!tokenizer_stream_is_empty(tok->stream))
    {
        token = tokenizer_next(tok, &sep);
        op = _op_sym_to_name(sep);
        if (op != NULL)
            g_string_append_printf(accu, "\"%s\" %s ",
                    token, op);
        else
            g_string_append_printf(accu, "\"%s\"", token);
        g_free(token);
    }
    tokenizer_destroy(tok);
    return g_string_free(accu, FALSE);
}

char *path_to_qstring (const char *path, gboolean is_file_path)
{
    char *qstring = NULL;

    log_msg("path_to_qstring( path = \"%s\", is_file_path = %s )\n",
            path, is_file_path?"TRUE":"FALSE");
    if (is_file_path)
    {
        char *basecopy = g_strdup(path);
        char *base = basename(basecopy);
        char *dircopy = g_strdup(path);
        char *dir = dirname(dircopy);

        if (g_strcmp0(dir, "@all") == 0)
            qstring = g_strdup_printf( "TAG TSPEC name=\"%s\"", base);
        else
            qstring = g_strdup_printf( "TAG TSPEC %s AND name=\"%s\"", dir, base);
        g_free(basecopy);
        g_free(dircopy);
        //g_free(dir);
    }
    else
    {
        char *q = translate_path(path);
        log_msg("Translated path = \"%s\"\n", q);
        qstring = g_strdup_printf("TAG TSPEC %s", q);
        g_free(q);
    }
    log_msg("Exiting path_to_qstring\n");
    return qstring;
}

int path_to_file_id (const char *path)
{
    char *qstring = path_to_qstring(path, TRUE);
    result_t *res = tagdb_query(TAGFS_DATA->db, qstring);
    g_free(qstring);
    if (res == NULL)
    {
        //log_msg("path_to_file_id got res==NULL\n");
        return 0;
    }

    if (res->type == tagdb_dict_t)
    {
        if (g_hash_table_size(res->data.d) != 0)
        {
            gpointer k, v;
            GHashTableIter it;
            g_hash_loop(res->data.d, it, k, v)
            {
                g_free(res);
                return TO_I(k);
            }
        } 
        else
        {
            g_free(res);
            return 0;
        }
    }
    return 0;
}

// turn the path into a file in the copies directory
// path_to_qstring + tagdb_query + tagfs_realpath
// NULL for a file that DNE
char *get_id_copies_path (const char *path)
{
    int id = path_to_file_id(path);
    if (id == 0)
        return NULL;
    int maxlen = 16;
    char id_string[maxlen];
    int length = g_snprintf(id_string, maxlen, "%d", id);
    if (length >= maxlen)
    {
        //log_msg("get_id_copies_path: id (%d) too long\n", id);
        exit(-1);
    }
    return tagfs_realpath(id_string);
}
