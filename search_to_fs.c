#include <string.h>
#include "query.h"
#include "types.h"
#include "params.h"
#include "search_to_fs.h"
#include "log.h"
// multiple searches are stored for lookup by their search strings
// the actual search result gets stored in search_fs_to_listable
// The search_str argument is the search string without any leading
// "FILE SEARCH", it may, however, be preceded by the :S prefix

// strips off everything up to and including the :S part
static char *fix_key (const char *dirty_key)
{
    char *res = g_strdup(strstr(dirty_key, SEARCH_PREFIX));
    char *slashp = strstr(res, "/");
    if (slashp)
        *slashp = '\0';
    if (res)
        return res + strlen(SEARCH_PREFIX);
    else
        return res;
}

SearchList *new_search_list ()
{
    return g_hash_table_new_full(g_str_hash, g_str_equal, NULL, (GDestroyNotify) search_result_destroy);
}

SearchResult *new_search_result ()
{
    return g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
}

void search_result_destroy (SearchResult *sr)
{
    g_hash_table_destroy((GHashTable*)sr);
}

static char *get_list_name (File *f, SearchResult *past_names)
{
    char *res = g_strdup(f->name);
    while (g_hash_table_lookup(past_names, res))
    {
        char *tmp = g_strdup_printf("_%s", res);
        g_free(res);
        res = tmp;
    }
    return res;
}

void search_fs_insert (const char *search_str)
{
    char *key = fix_key(search_str);
    log_msg("the fucking key = %s\n", key);
    char *qstring = g_strdup_printf("FILE SEARCH %s", key);
    result_t *qres = tagdb_query(DB, qstring);
    SearchResult *sr = new_search_result();
    GHashTable *dict = tagdb_value_extract_dict(qres);
    if (dict)
    {
        HL(dict, it, k, v)
        {
            file_id_t id = tagdb_value_extract_int(k);
            File *f = retrieve_file(DB, id);
            char *name = get_list_name(f, sr);
            g_hash_table_insert(sr, name, f);
        } HL_END;
    }
    g_hash_table_insert(SEARCHES, key, sr);
    g_free(qstring);
    result_destroy(qres);
}

void search_fs_readdir (const char *search_str, fuse_fill_dir_t filler, void *fill_buffer)
{
    char *key = fix_key(search_str);
    SearchResult *sr = g_hash_table_lookup(SEARCHES, key);
    HL (sr, it, k, v)
    {
        filler(fill_buffer, k, NULL, 0);
    } HL_END;
}

/* saves us the trouble of writing a special stat method ... */
File *search_fs_get_file (const char *path)
{
    char *key = fix_key(path);
    char *fname = g_path_get_basename(path);
    log_msg("god, i hate myself %s\n", key);
    SearchResult *sr = g_hash_table_lookup(SEARCHES, key);
    File *res = g_hash_table_lookup(sr, fname);

    g_free(fname);
    //g_free(key);

    return res;
}
