#ifndef COMMAND_DEFAULT_H
#define COMMAND_DEFAULT_H
#include "command.h"

#define COMMAND_NAME_SIZE 16
#define COMMAND_MAX_ARGS 128

typedef enum {
    TAGFS_COMMAND_DEFAULT_ERROR_NO_SUCH_COMMAND
} TagFSCommandDefaultError;

enum command_t {
    TALS,
    LPOS,
    COMMAND_MAX
};

typedef int (*command_func) (int argc, const char **argv, GString *out, GError **err);
void command_default_handler (CommandResponse *resp, CommandRequest *req, GError **err);

#endif /* COMMAND_DEFAULT_H */

