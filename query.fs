#include <string.h>
#include <errno.h>
#include "log.h"
#include "params.h"
#include "path_util.h"
#include "types.h"
#include "query_fs_result_manager.h"
#include "result_to_fs.h"
#include "query.h"

const char *path_to_result_path (const char *path)
{
    char *res_part = g_strrstr(path, LISTEN_FH);
    return strstr(res_part, "/");
}

char *path_to_query (const char *path)
{
    /* /path/to/:L<Query>/<Rest> */
    log_msg("initial string \"%s\"\n", path);

    char *query_start = strstr(path, "/" LISTEN_FH); /* /:L<Query>/<Rest> */
    query_start += strlen(LISTEN_FH) + 1; /* <Query>/<Rest> */
    int len = strcspn(query_start, "/"); /* /<Rest> */
    if (len == 0) return NULL;
    char *query = g_malloc(len + 1); /* size strlen(<Query>) */
    strncpy(query, query_start, len);
    query[len+1] = '\0';
    return query;
}

/* If there isn't a value yet, we do the query
 * and get a value */
result_t *my_query_result_lookup (const char *path)
{
    char *qstr = path_to_query(path);
    result_t *r = NULL;
    log_msg("mqrl qstr=\"%s\"\n", qstr);
    if (!qstr)
    {
        r = query_result_retrieve_contents(FSDATA->rqm);
    }
    else
    {
        r = query_result_lookup(FSDATA->rqm, qstr);
        if (!r)
        {
            r = tagdb_query(DB, qstr);
            query_result_insert(FSDATA->rqm, qstr, r);
        }
    }
    g_free(qstr);
    return r;
}

result_t *path_to_result (const char *path)
{
    result_t *res = NULL;
    result_t *qres = my_query_result_lookup(path);

    if (qres)
    {
        const char *res_part = path_to_result_path(path);
        log_msg("result_path = %s\n", res_part);
        res = result_fs_path_to_result(qres, res_part);
    }
    return res;
}

pcheck%path%
{
    return path_has_component_with_prefix(path, LISTEN_FH);
}

op%getattr path statbuf%
{
    int retstat = -ENOENT;
    if (g_str_has_suffix(path, "/" LISTEN_FH))
    {
        statbuf->st_mode = S_IFREG | 0755;
        statbuf->st_size = 0;
        retstat = 0;
    }
    else
    {
        result_t *res = path_to_result(path);
        result_fs_getattr(res, statbuf);
        retstat = 0;
    }
    return retstat;
}

op%readdir path buffer filler offset f_info%
{
    int retstat = 0;
    char *qstr = path_to_query(path);

    if (g_str_has_suffix(path, qstr))
    {
        query_result_insert(FSDATA->rqm, qstr, tagdb_query(DB, qstr));
    }
    result_t *r = path_to_result(path);
    if (!r) retstat = -1;
    result_fs_readdir(r, buffer, filler);

    g_free(qstr);
    return retstat;
}

op%read path, buf, size, offset, f_info%
{
    result_t *r = path_to_result(path);
    return result_fs_read(r, buf, size, offset);
}

op%write path buf size offset f_info%
{
    // sanitize the buffer string
    char *cmdstr = g_strstrip((char*)g_memdup(buf, size + 1));
    cmdstr[size] = '\0';
    result_t *res = query_and_queue_result(cmdstr);
    res_info(res, log_msg0);
    g_free(cmdstr);
    return size;
}

%%%register_component%%%
