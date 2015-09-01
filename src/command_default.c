#include <string.h>
#include "command_default.h"

command_func commands[COMMAND_MAX + 1];
int command_argcs[COMMAND_MAX + 1];


int command_idx(char *command_name)
{
    for (int i = 0; i < COMMAND_MAX; i++)
    {
        if (strncmp(command_name, command_names[i], COMMAND_NAME_SIZE) == 0)
        {
            return i;
        }
    }
    return COMMAND_MAX;
}

int call (const char *buf)
{
    int argc;
    char **argv;
    GError *err;
    g_shell_parse_argv(buf, &argc, &argv, &err);
    if (err != NULL)
    {
        return 1;
    }

    int cmd = command_idx(argv[0]);
    if (cmd == COMMAND_MAX)
    {
        return 1;
    }
    else
    {
        commands[cmd](argc, (const char**) argv);
    }
    g_strfreev(argv);
    return 0;
}

command_func commands[COMMAND_MAX + 1] =
{
    [TALS] = alias_tag,
    [COMMAND_MAX] = NULL
};

int command_argcs[COMMAND_MAX + 1] =
{
    [TALS] = 2,
    [COMMAND_MAX] = 0
};

