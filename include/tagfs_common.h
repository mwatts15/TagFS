#ifndef TAGFS_COMMON_H
#define TAGFS_COMMON_H
struct tagfs_state *process_options (int *argc, char ***argv);
struct tagfs_state *process_options0 (int *argcp, char ***argvp, int optbuf);

#endif /* TAGFS_COMMON_H */

