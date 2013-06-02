#ifndef TAGFS_H
#define TAGFS_H

#include "log.h"
#include "tagdb.h"
#include "stage.h"
#include "query_fs_result_manager.h"
#include "search_fs.h"

struct tagfs_state
{
    char *copiesdir;
    char *log_file;
    TagDB *db;
    Stage *stage;
    /* The result from the last search performed
       a list of files */
    SearchList *search_results;
    QueryResultManager *rqm;
};

gboolean tagfs_is_consistent ();
void toggle_tagfs_consistency ();
void ensure_tagfs_consistency (struct tagfs_state *st);

#endif /* TAGFS_H */
