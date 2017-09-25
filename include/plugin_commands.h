#ifndef PLUGIN_COMMANDS_H
#define PLUGIN_COMMANDS_H
#include <glib.h>
typedef enum {
    TAGFS_PLUGIN_COMMAND_ERROR_FAILED
} TagFSPluginCommandError;

int register_plugin_command (int argc, const char **argv, GString *out, GError **err);
int unregister_plugin_command (int argc, const char **argv, GString *out, GError **err);

#endif /* PLUGIN_COMMANDS_H */

