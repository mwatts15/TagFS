#include <string.h>
#include "command_default.h"
#include "tagdb_commands.h"

command_func commands[COMMAND_MAX + 1];
int command_argcs[COMMAND_MAX + 1];
char command_names[COMMAND_MAX][COMMAND_NAME_SIZE];

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

int call (const char *buf, GString *out, GError **err)
{
    int retstat = 0;
    int argc;
    char **argv;
    g_shell_parse_argv(buf, &argc, &argv, err);
    if ((err != NULL) && (*err != NULL))
    {
        retstat = 1;
    }
    else
    {
        int cmd = command_idx(argv[0]);
        if (cmd == COMMAND_MAX)
        {
            retstat = 1;
        }
        else
        {
            commands[cmd](argc, (const char**) argv, out, err);
        }
    }
    g_strfreev(argv);
    return retstat;
}

void command_default_handler(CommandResponse *resp, CommandRequest *req, GError **err)
{
    call(command_buffer(req)->str, command_buffer(resp), err);
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


char command_names[][COMMAND_NAME_SIZE] =
{
    "alias_tag"
};
