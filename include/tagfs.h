#ifndef TAGFS_H
#define TAGFS_H

#include "log.h"
#include "tagdb.h"
#include "stage.h"
#include "command.h"
#include "message.h"

struct tagfs_state
{
    char *copiesdir;
    char *log_file;
    char *pid_file;
    TagDB *db;
    Stage *stage;
    CommandManager *command_manager;
    MessageConnection *mess_conn;
};

gboolean tagfs_is_consistent ();
void toggle_tagfs_consistency ();
void ensure_tagfs_consistency (struct tagfs_state *st);

#endif /* TAGFS_H */
