#include <glib.h>
#include <string.h>
#include "version.h"
#include "tagdb.h"
#include "tagdb_util.h"
#include "tag.h"
#include "command_default.h"
#include "tagdb_commands.h"
#include "params.h"

#define TAGFS_TAGDB_COMMAND_ERROR tagfs_tagdb_command_error_quark ()

GQuark tagfs_tagdb_command_error_quark ()
{
    return g_quark_from_static_string("tagfs-tagdb-command-error-quark");
}

int alias_tag_command (int argc, const char **argv, GString *out, GError **err)
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

    if (strlen(alias) == 0) {
        g_set_error(err, TAGFS_TAGDB_COMMAND_ERROR,
                TAGFS_TAGDB_COMMAND_ERROR_FAILED,
                "Tag alias command failed. Alias cannot be an empty string.");
        return -1;
    }

    Tag *t = tagdb_lookup_tag(DB, tag_name);
    if (!t)
    {
        g_set_error(err, TAGFS_TAGDB_COMMAND_ERROR,
                TAGFS_TAGDB_COMMAND_ERROR_FAILED,
                "Tag alias command failed. tagdb_lookup_tag failed.");
        return -1;

    }

    if (!tagdb_alias_tag(DB, t, alias))
    {
        g_set_error(err, TAGFS_TAGDB_COMMAND_ERROR,
                TAGFS_TAGDB_COMMAND_ERROR_FAILED,
                "Tag alias command failed. tagdb_alias_tag failed.");
        return -1;
    }
    g_string_append_printf(out, "Aliased %s to %s\n", tag_name, alias);
    return 0;
}

int tag_command (int argc, const char **argv, GString *out, GError **err)
{
    if (argc < 1)
    {
        g_set_error(err, TAGFS_TAGDB_COMMAND_ERROR,
                TAGFS_TAGDB_COMMAND_ERROR_FAILED,
                "Insufficient number of arguments to tag command\n"
                "Usage: tag tag_name default_explanation");
        return -1;
    } else if (argc > 3) {
        g_set_error(err, TAGFS_TAGDB_COMMAND_ERROR,
                TAGFS_TAGDB_COMMAND_ERROR_FAILED,
                "Too many arguments to tag command\n"
                "Usage: tag tag_name default_explanation");
        return -1;
    }

    const char *tag_name = argv[1];
    const char *default_explanation = argv[2];

    Tag *t = tagdb_lookup_tag(DB, tag_name);

    if (!t)
    {
        if (!tagdb_make_tag0(DB, tag_name, default_explanation))
        {
            g_set_error(err, TAGFS_TAGDB_COMMAND_ERROR,
                    TAGFS_TAGDB_COMMAND_ERROR_FAILED,
                    "Failed to create tag.");
            return -1;
        }
    }
    t = tagdb_lookup_tag(DB, tag_name);
    char *tmp;
    tmp = g_strescape(tag_name(t), "");
    g_string_append_printf(out, "- name: %s\n", tmp);
    g_free(tmp);

    tmp = g_strescape(tag_default_explanation(t), "");
    g_string_append_printf(out, "  default_explanation: %s\n", tmp);
    g_free(tmp);
    return 0;
}

int info_command (int argc, const char **argv, GString *out, GError **err)
{
    char *tmp;

    tmp = g_strescape(TAGFS_VERSION, "");
    g_string_append_printf(out, "version: \"%s\"\n", tmp);
    g_free(tmp);

    g_string_append_printf(out, "number_of_files: %lu\n", tagdb_nfiles(DB));
    g_string_append_printf(out, "number_of_tags: %lu\n", tagdb_ntags(DB));

    tmp = g_strescape(DB->sqlite_db_fname, "");
    g_string_append_printf(out, "database_file: \"%s\"\n", tmp);
    g_free(tmp);

    g_string_append(out, "commands:\n");
    for (int i = 0 ; i < COMMAND_MAX; i++)
    {
        g_string_append_printf(out, "    - name: %s\n", command_names[i]);
        tmp = g_strescape(command_descriptions[i], "");
        g_string_append_printf(out, "      description: \"%s\"\n", tmp);
        g_free(tmp);
        g_string_append_printf(out, "      argument_count: %d\n", command_argcs[i]);
    }
    g_string_append(out, "addons: []\n");
    return 0;
}

int list_position_command (int argc, const char **argv, GString *out, GError **err)
{
    tagdb_key_t k = key_new();
    char *tmp;
    for (int i = 1; i < argc; i++)
    {
        const char *tag_name = argv[i];
        Tag *t = tagdb_lookup_tag(DB, tag_name);
        if (t)
        {
            key_push_end(k, tag_id(t));
        }
        else
        {
            key_destroy(k);
            tmp = g_strescape(tag_name, "");
            g_set_error(err, TAGFS_TAGDB_COMMAND_ERROR,
                    TAGFS_TAGDB_COMMAND_ERROR_FAILED,
                    "Tag \"%s\" is not known.", tmp);
            g_free(tmp);
            return -1;
        }
    }
    char fname[MAX_FILE_NAME_LENGTH];
    GList *tags = get_tags_list(DB, k);
    GList *files = get_files_list(DB, k);
    GList *stage_tag_ids = stage_list_position(STAGE, k);
    g_string_printf(out, "tags:\n");
    LL(tags, it)
    {
        g_string_append(out, "    - [");
        tag_to_string1(it->data, fname, MAX_FILE_NAME_LENGTH);
        tmp = g_strescape(fname, "");
        g_string_append_printf(out, "\"%s\"", tmp);
        g_free(tmp);

        SLL(tag_aliases(it->data), ita)
        {
            char *name = (char*)ita->data;
            tmp = g_strescape(name, "");
            g_string_append_printf(out, ", \"%s\"", tmp);
            g_free(tmp);
        } SLL_END
        g_string_append(out, "]\n");
    } LL_END

    g_string_append_printf(out, "staged_tags:\n");
    LL(stage_tag_ids, it)
    {
        Tag *t = retrieve_tag(DB, TO_S(it->data));
        if (t)
        {
            tag_to_string1(t, fname, MAX_FILE_NAME_LENGTH);
            tmp = g_strescape(fname, "");
            g_string_append_printf(out, "    - \"%s\"\n", tmp);
            g_free(tmp);
        }
    } LL_END

    g_string_append_printf(out, "files:\n");
    LL(files, it)
    {
        file_to_string(it->data, fname);
        tmp = g_strescape(fname, "");
        g_string_append_printf(out, "    - \"%s\"\n", tmp);
        g_free(tmp);
    } LL_END

    g_list_free(stage_tag_ids);
    g_list_free(files);
    g_list_free(tags);

    return 0;
}
