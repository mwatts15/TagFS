#ifndef COMMAND_DEFAULT_H
#define COMMAND_DEFAULT_H
#include "command.h"

#define COMMAND_NAME_SIZE 32
#define COMMAND_MAX_ARGS 128
#define COMMAND_DESCRIPTION_SIZE 256

typedef enum {
    TAGFS_COMMAND_DEFAULT_ERROR_NO_SUCH_COMMAND
} TagFSCommandDefaultError;

enum command_t {
    TALS,
    LPOS,
    TINF,
    NTAG,
    RPLU,
    UNRP,
    COMMAND_MAX
};

typedef int (*command_func) (int argc, const char **argv, GString *out, GError **err);
void command_default_handler (CommandResponse *resp, CommandRequest *req, GError **err);
extern char command_names[COMMAND_MAX][COMMAND_NAME_SIZE];
extern char command_descriptions[COMMAND_MAX][COMMAND_DESCRIPTION_SIZE];

/** Table of the maximum number of arguments the command accepts. Purely informational */
extern int command_argcs[COMMAND_MAX + 1];

#endif /* COMMAND_DEFAULT_H */

