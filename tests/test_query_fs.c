#include <string.h>
#include "log.h"
#include "test_util.h"
#include "query_fs.h"

/* Test the path utilities */
int main ()
{
    open_test_log();
    struct tagfs_state *tagfs_data = g_try_malloc(sizeof(struct tagfs_state));
    tagfs_data->db = tagdb_load("test.db");
    tagfs_data->rqm = query_result_manager_new();
    fuse_init(tagfs_data);

    log_msg("handles path1 = %d\n",
            query_fs_handles_path("/tag/tag0/" LISTEN_FH));
    log_msg("handles path2 = %d\n",
            query_fs_handles_path("/tag/tag0/" LISTEN_FH "FILE LIST"));

    struct stat stat;
    memset(&stat, 0, sizeof(struct stat));
    query_fs_getattr("/path/" LISTEN_FH "FILE SEARCH tag001", &stat);
    log_stat(&stat);

    query_fs_readddir("/path/" LISTEN_FH "FILE SEARCH tag001", NULL, fake_fuse_dir_filler, 0, NULL);
    query_fs_readddir("/path/" LISTEN_FH "FILE SEARCH tag001/4", NULL, fake_fuse_dir_filler, 0, NULL);
    query_fs_readddir("/path/" LISTEN_FH "FILE SEARCH tag001/5", NULL, fake_fuse_dir_filler, 0, NULL);
    query_fs_readddir("/path/" LISTEN_FH "FILE SEARCH tag001/6", NULL, fake_fuse_dir_filler, 0, NULL);
    query_fs_readddir("/path/" LISTEN_FH "FILE SEARCH tag001/10", NULL, fake_fuse_dir_filler, 0, NULL);
    query_fs_readddir("/path/" LISTEN_FH "FILE SEARCH tag001/11", NULL, fake_fuse_dir_filler, 0, NULL);
    query_fs_readddir("/path/" LISTEN_FH "FILE SEARCH tag001", NULL, fake_fuse_dir_filler, 0, NULL);
    log_close();
    return 0;
}
/* Test the other stuff */
