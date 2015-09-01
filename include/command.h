#ifndef COMMAND_H
#define COMMAND_H
#include <glib.h>

#define COMMAND_NAME_SIZE 12
#define COMMAND_MAX_ARGS 128
#define COMMAND_MAX_REQUESTS 64

typedef struct CommandRequest
{
    /** The kind of command */
    char *kind;
    /** Key for the command */
    char *key;
    /** The buffer where commands are written */
    GString *command_buffer;
} CommandRequest;

typedef struct CommandResponse
{
    /** The kind of command */
    char *kind;
    /** Key for the command */
    char *key;
    /** The buffer where results from a command are read */
    GString *result_buffer;
} CommandResponse;

/** Contains information for choosing how to execute a command */
typedef struct CommandManager
{
    /** A mapping from command type to command procedure */
    GHashTable *command_table;
    /** A mapping from keys to CommandRequests */
    GHashTable *requests;
    /** A mapping from keys to CommandResponses */
    GHashTable *responses;
} CommandManager;

typedef CommandResponse * (*command_do)(CommandRequest *cr, GError**);

#endif /* COMMAND_H */

