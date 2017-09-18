#include "command_default.h"
#include "params.h"

#define TAGFS_PLUGIN_COMMAND_ERROR tagfs_plugin_command_error_quark ()
#define TAGFS_COMMAND_REGISTER_PLUGIN_USAGE \
            "Usage: register_plugin name interface [reconnect_policy]\n" \
            "     reconnect_policy := (\"on-demand\"|\"on-notify\"|\"manual\"|\"\")"

GQuark tagfs_plugin_command_error_quark ()
{
    return g_quark_from_static_string("tagfs-plugin-command-error-quark");
}

int register_plugin_command (int argc, const char **argv, GString *out, GError **err)
{
    if (argc < 3)
    {
        g_set_error(err,
                TAGFS_PLUGIN_COMMAND_ERROR,
                TAGFS_PLUGIN_COMMAND_ERROR_FAILED,
                "Insufficient number of arguments to register_plugin command\n"
                TAGFS_COMMAND_REGISTER_PLUGIN_USAGE
                usage);
        return -1;
    }

    const char *name = argv[1];
    const char *interface = argv[2];

    if (!g_dbus_is_name(name) || !g_dbus_is_interface_name(interface))
    {
        g_set_error(err,
                TAGFS_PLUGIN_COMMAND_ERROR,
                TAGFS_PLUGIN_COMMAND_ERROR_FAILED,
                "Either the plugin name or the interface name is invalid\n"
                TAGFS_COMMAND_REGISTER_PLUGIN_USAGE
                );
        return -1;
    }

    const char *reconnect_policy_string;
    PluginReconnectPolicy reconnect_policy = PLUGIN_RECONNECT_MANUAL;

    if (argc > 3)
    {
        reconnect_policy_string = argv[3];
        gboolean good_policy = FALSE;
        for (int i = 0; i < N_PLUGIN_RECONNECT_POLICIES; i++)
        {
            if (strncmp(reconnect_policy_string,
                        reconnect_policy_strings[i],
                        strlen(reconnect_policy_strings[i])) == 0)
            {
                good_policy = TRUE;
                reconnect_policy = i;
                break;
            }
        }

        if (!good_policy)
        {
            g_set_error(err,
                    TAGFS_PLUGIN_COMMAND_ERROR,
                    TAGFS_PLUGIN_COMMAND_ERROR_FAILED,
                    "The given reconnect policy is invalid\n"
                    TAGFS_COMMAND_REGISTER_PLUGIN_USAGE);
            return -1;
        }
    }

    plugin_manager_register_plugin0(PM, name, interface, reconnect_policy);
}
