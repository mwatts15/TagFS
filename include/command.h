#ifndef COMMAND_H
#define COMMAND_H
#include <glib.h>

#define COMMAND_MAX_REQUESTS 64

typedef enum {
    TAGFS_COMMAND_ERROR_NO_SUCH_COMMAND
} TagFSCommandError;

typedef struct CommandRequest
{
    /** The kind of command */
    const char *kind;
    /** A unique key for the command */
    const char *key;
    /** The buffer where commands are written */
    GString *command_buffer;
} CommandRequest;

typedef struct CommandResponse
{
    /** The kind of command */
    const char *kind;
    /** A unique key for the command */
    const char *key;
    /** The buffer where results from a command are read */
    GString *result_buffer;
} CommandResponse;

/** Contains information for choosing how to execute a command */
typedef struct CommandManager
{
    /** A mapping from command "kind" to command procedure */
    GHashTable *command_table;
    /** A mapping from keys to CommandRequests */
    GHashTable *requests;
    /** A mapping from keys to CommandResponses */
    GHashTable *responses;
    /** A mapping from keys to errors in processing commands */
    GHashTable *errors;
} CommandManager;

#define command_response_from_request(__cresp, __creq) \
do {\
    __cresp->kind = g_strdup(__creq->kind);\
    __cresp->key = g_strdup(__creq->key);\
} while (0)

struct ReadParams
{
    char *buf;
    size_t size;
    size_t offset;
};

struct WriteParams
{
    const char *buf;
    size_t size;
    size_t offset;
};

/** The type for CommandRequest handlers.
 *
 * The handlers are not required to check for the GError** being properly
 * initialized.
 *
 * A handler should not free the CommandRequest* or the CommandResponse.
 *
 * If the GError** is filled in, then the response object will be ignored.
 */
typedef void (*command_do)(CommandResponse *resp, CommandRequest *req, GError**);

CommandManager *command_init();
void command_manager_destroy(CommandManager *cm);
CommandManager *command_manager_new();
/** Register a handler for the kind of command */
void command_manager_handler_register(CommandManager *cm, const char *kind, command_do func);
command_do command_manager_get_handler(CommandManager *cm, const char *kind);
/** Handles a CommandRequest, destroys the request, and stores the response in
 * the CommandManager
 */
void command_manager_handle(CommandManager *cm, CommandRequest *req);
/** Retrieves a CommandRequest, and handles it as command_manager_handle */
void command_manager_handle_request(CommandManager *cm, const char *key);
CommandRequest *command_request_new();
CommandResponse *command_response_new();
CommandRequest *command_request_new2(const char *kind, const char *key);
CommandResponse *command_response_new2(const char *kind, const char *key);
void command_request_destroy (CommandRequest *cr);
void command_response_destroy (CommandResponse *cr);
ssize_t command_write_response(CommandResponse *resp, struct WriteParams wd);
ssize_t command_read_response(CommandResponse *resp, struct ReadParams rd);
ssize_t command_write_request(CommandRequest *resp, struct WriteParams wd);
ssize_t command_read_request(CommandRequest *resp, struct ReadParams rd);
/** Creates a new request managed by the command manager.
 *
 * If the kind is NULL, then the default kind is assumed.
 * If the key is NULL, then no request will be created.
 */
CommandRequest *command_manager_request_new(CommandManager *cm, const char *kind, const char *key);
CommandResponse *command_manager_response_new(CommandManager *cm, CommandRequest* cr);
/** Get a response that's been handled previously */
CommandResponse *command_manager_get_response(CommandManager *cm, const char *key);
CommandRequest* command_manager_get_request(CommandManager *cm, const char *key);
size_t command_request_size(CommandRequest *req);
size_t command_response_size(CommandResponse *res);

#endif /* COMMAND_H */
