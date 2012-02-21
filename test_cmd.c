#include <stdio.h>
#include "tagdb.h"

const char *commands[] = {"add_file", "add_file_tag", NULL};

// encode the command name
int _name_to_code (const char *name)
{
    // remember, lists and iteration solve everything!
    int i = 0;
    while (commands[i] != NULL)
    {
        if (g_strcmp0(commands[i], name) == 0)
            return i;
        i++;
    }
    return -1;
}

void _do_cmd (tagdb *db, int cmd, const char **args)
{
    switch (cmd)
    {
        case 0: // add file
            tagdb_insert_file(db, args[0]);
            break;
        case 1: // add tag to file
            tagdb_insert_file_tag(db, args[0], args[1]);
            break;
        default:
            // do nothing
            break;
    }
}

int do_cmd (tagdb *db, const char *cmd, const char **args)
{
    int code = _name_to_code(cmd);
    if (code == -1)
    {
        return code;
    }
    _do_cmd(db, code, args);
    return 0;
}

int main ()
{
    tagdb *db = newdb("test.db", "tags.list");
    print_list(stdout, tagdb_files(db));
    const char *cmd_args[] = {"newfile"};
    do_cmd(db, "add_file", cmd_args);
    print_list(stdout, tagdb_files(db));
    printf("TAGS: ");
    print_list(stdout, tagdb_tagstruct(db));
    const char *cmd_args1[] = {"newfile", "horse"};
    do_cmd(db, "add_file_tag", cmd_args1);
    print_list(stdout, tagdb_tagstruct(db));
}
