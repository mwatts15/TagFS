#include "params.h"
// multiple searches are stored for lookup by their search strings
// the actual search result gets stored in search_fs_to_listable
// The search_str argument is the search string without any leading
// "FILE SEARCH", it may, however, be preceded by the :S prefix
void search_fs_insert (const char *search_str)
{
    char *qstring = g_strdup_printf("FILE SEARCH %s", search_str + strlen(SEARCH_PREFIX));
    // transform and store
}

void search_fs_readdir (const char *search_str, fuse_fill_dir_t filler)
{
    // fill in from the stored search
}

/* The path must begin with the search string but may have
 * a leading "/" as well */
void search_fs_stat (const char *path, struct stat *statbuf)
{
    // fill in the statbuf for a file in the stored search
}
