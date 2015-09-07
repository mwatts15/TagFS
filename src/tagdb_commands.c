#include <glib.h>
#include "tagdb.h"
#include "tag.h"
#include "command_default.h"
#include "tagdb_commands.h"
#include "params.h"

#define TAGFS_TAGDB_COMMAND_ERROR tagfs_tagdb_command_error_quark ()

GQuark tagfs_tagdb_command_error_quark ()
{
    return g_quark_from_static_string("tagfs-tagdb-command-error-quark");
}

int alias_tag (int argc, const char **argv, GString *out, GError **err)
{
    if (argc < 2)
    {
        g_set_error(err, TAGFS_TAGDB_COMMAND_ERROR,
                TAGFS_TAGDB_COMMAND_ERROR_FAILED,
                "Insufficient number of arguments to alias_tag");
        return -1;
    }

    const char *tag_name = argv[1];
    const char *alias = argv[2];
    Tag *t = tagdb_lookup_tag(DB, tag_name);
    if (!t)
    {
        g_set_error(err, TAGFS_TAGDB_COMMAND_ERROR,
                TAGFS_TAGDB_COMMAND_ERROR_FAILED,
                "Tag alias command failed");
        return -1;

    }

    if (!tagdb_alias_tag(DB, t, alias))
    {
        g_set_error(err, TAGFS_TAGDB_COMMAND_ERROR,
                TAGFS_TAGDB_COMMAND_ERROR_FAILED,
                "Tag alias command failed");
        return -1;
    }
    g_string_append_printf(out, "Aliased %s to %s\n", tag_name, alias);
    return 0;
}

