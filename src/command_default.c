#include <string.h>
#include "command_default.h"
#include "tagdb_commands.h"

command_func commands[COMMAND_MAX + 1];
int command_argcs[COMMAND_MAX + 1];
char command_names[COMMAND_MAX][COMMAND_NAME_SIZE];

#define TAGFS_COMMAND_DEFAULT_ERROR tagfs_command_default_error_quark ()

GQuark tagfs_command_default_error_quark ()
{
    return g_quark_from_static_string("tagfs-command-default-error-quark");
}

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

int call (char *buf, GString *out, GError **err)
{
    int retstat = 0;
    int argc;
    char **argv;
    g_strstrip(buf);
    if (!g_shell_parse_argv(buf, &argc, &argv, err))
    {
        retstat = 1;
    }
    else
    {
        int cmd = command_idx(argv[0]);
        if (cmd == COMMAND_MAX)
        {
            g_set_error(err,
                    TAGFS_COMMAND_DEFAULT_ERROR,
                    TAGFS_COMMAND_DEFAULT_ERROR_NO_SUCH_COMMAND,
                    "'%s' is not a known command", argv[0]);
            retstat = 1;
        }
        else
        {
            commands[cmd](argc, (const char**) argv, out, err);
        }
        g_strfreev(argv);
    }

    return retstat;
}

void command_default_handler(CommandResponse *resp, CommandRequest *req, GError **err)
{
    call(command_buffer(req)->str, command_buffer(resp), err);
}

command_func commands[COMMAND_MAX + 1] =
{
    [TALS] = alias_tag_command,
    [LPOS] = list_position_command,
    [TINF] = info_command,
    [COMMAND_MAX] = NULL
};

int command_argcs[COMMAND_MAX + 1] =
{
    [TALS] = 2,
    [LPOS] = -1,
    [TINF] = 0,
    [COMMAND_MAX] = 0
};


char command_names[][COMMAND_NAME_SIZE] =
{
    [TALS] = "alias_tag",
    [LPOS] = "list_position",
    [TINF] = "info"
};

char command_descriptions[][COMMAND_DESCRIPTION_SIZE] =
{
    [TALS] = "Add an alias for the tag",
    [LPOS] = "List the tags and files associated to a set of tags",
    [TINF] = "Show the info for the mounted TagFS"
};
