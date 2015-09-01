#ifndef COMMAND_DEFAULT_H
#define COMMAND_DEFAULT_H
#include "command.h"

enum command_t {
    TALS,
    COMMAND_MAX
};

char command_names[][COMMAND_NAME_SIZE] =
{
    "alias_tag"
};

typedef int (*command_func) (int argc, const char **argv, GString *out, GError **err);

#endif /* COMMAND_DEFAULT_H */

