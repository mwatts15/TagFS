#ifndef TAGDB_COMMANDS_H
#define TAGDB_COMMANDS_H
#include <glib.h>
typedef enum {
    TAGFS_TAGDB_COMMAND_ERROR_FAILED
} TagFSTagDBCommandError;

int alias_tag (int argc, const char **argv, GString *out, GError **err);

#endif /* TAGDB_COMMANDS_H */

