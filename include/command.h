#ifndef COMMAND_H
#define COMMAND_H
#include <glib.h>
#include <semaphore.h>

#define COMMAND_MAX_REQUESTS 64

typedef enum {
    TAGFS_COMMAND_ERROR_NO_SUCH_COMMAND
} TagFSCommandError;

typedef struct CommandRequestResponse
{
    /** The kind of command */
    const char *kind;
    /** A unique key for the command */
    const char *key;
    /** The buffer where commands are written */
    GString *buffer;
    /** The lock for reads/writes to the buffer */
    sem_t lock;
} CommandRequestResponse;

typedef struct CommandRequest
{
    CommandRequestResponse base;
} CommandRequest;

typedef struct CommandResponse
{
    CommandRequestResponse base;
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
CommandRequest *command_request_new();
CommandResponse *command_response_new();
CommandRequest *command_request_new2(const char *kind, const char *key);
CommandResponse *command_response_new2(const char *kind, const char *key);
void command_request_destroy (CommandRequest *cr);
void command_response_destroy (CommandResponse *cr);
ssize_t _command_read(CommandRequestResponse *resp, struct ReadParams rd);
ssize_t _command_write(CommandRequestResponse *resp, struct WriteParams wd);
/** Creates a new request managed by the command manager.
 *
 * If the kind is NULL, then the default kind is assumed.
 * If the key is NULL, then no request will be created.
 */
CommandRequest *command_manager_request_new(CommandManager *cm, const char *kind, const char *key);
CommandResponse *command_manager_response_new(CommandManager *cm, CommandRequest* cr);
/** Destroy the response for the key and remove it from the manager */
void command_manager_response_destroy (CommandManager *cm, const char *key);
/** Destroy the request for the key and remove it from the manager */
void command_manager_request_destroy (CommandManager *cm, const char *key);
/** Destroy the error and remove it from the manager */
void command_manager_error_destroy (CommandManager *cm, const char *key);
/** Get a response that's been handled previously */
CommandResponse *command_manager_get_response(CommandManager *cm, const char *key);
CommandRequest* command_manager_get_request(CommandManager *cm, const char *key);
GError* command_manager_get_error(CommandManager *cm, const char *key);
size_t _command_size(CommandRequestResponse *req);

#define command_buffer(cr) (((CommandRequestResponse*)(cr))->buffer)
#define command_key(cr) (((CommandRequestResponse*)(cr))->key)
#define command_kind(cr) (((CommandRequestResponse*)(cr))->kind)
#define command_lock(cr) lock_acquire(&((CommandRequestResponse*)(cr))->lock, 1)
#define command_unlock(cr) lock_release(&((CommandRequestResponse*)(cr))->lock)
#define command_write(cr, wp) _command_write((CommandRequestResponse*)(cr), (wp))
#define command_read(cr, rp) _command_read((CommandRequestResponse*)(cr), (rp))
#define command_size(cr) _command_size((CommandRequestResponse*)(cr))

#endif /* COMMAND_H */
