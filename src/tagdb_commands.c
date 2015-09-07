#include <glib.h>
#include "tagdb.h"
#include "tag.h"
#include "command_default.h"
#include "params.h"

int alias_tag (int argc, const char **argv, GString *out, GError **err)
{
    if (argc < 2)
    {
        return -1;
    }
    const char *tag_name = argv[1];
    const char *alias = argv[2];
    Tag *t = tagdb_lookup_tag(DB, tag_name);
    tagdb_alias_tag(DB, t, alias);
    return 0;
}

