#ifndef COMMAND_FS_H
#define COMMAND_FS_H
#include "subfs.h"
#define COMMAND_BASENAME "/.__cmd"
#define COMMAND_BASENAME_LEN 7
#define COMMAND_RES_BASENAME "/.__res"
#define COMMAND_RES_BASENAME_LEN 7
#define is_command_path(path) (strncmp(path, COMMAND_BASENAME, COMMAND_BASENAME_LEN) == 0)
#define is_command_response_path(path) (strncmp(path, COMMAND_RES_BASENAME, COMMAND_RES_BASENAME_LEN) == 0)
extern subfs_component command_fs_subfs;

#endif /* COMMAND_FS_H */

