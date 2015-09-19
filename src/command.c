#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tagdb.h"
#include "command.h"
#include "util.h"
#include "log.h"

ssize_t _write(GString *str, struct WriteParams wd);
ssize_t _read(GString *str, struct ReadParams rd);
#define TAGFS_COMMAND_ERROR tagfs_command_error_quark ()

GQuark tagfs_command_error_quark ()
{
    return g_quark_from_static_string("tagfs-command-error-quark");
}

GString *command_output(CommandResponse *cr)
{
    return command_buffer(cr);
}

void command_request_response_init(CommandRequestResponse *cr)
{
    command_buffer(cr) = g_string_new(NULL);
    sem_init(&cr->lock, 0, 1);
}

void command_request_response_init2(CommandRequestResponse *cr, const char *kind, const char *key)
{
    command_request_response_init(cr);
    command_key(cr) = g_strdup(key);
    command_kind(cr) = g_strdup(kind);
}

CommandRequest *command_request_new()
{
    CommandRequest *cr = g_malloc0(sizeof(struct CommandRequest));
    command_request_response_init((CommandRequestResponse*)cr);
    return cr;
}

CommandRequest *command_request_new2(const char *kind, const char *key)
{
    CommandRequest *cr = g_malloc0(sizeof(struct CommandRequest));
    command_request_response_init2((CommandRequestResponse*)cr, kind, key);
    return cr;
}

CommandResponse *command_response_new()
{
    CommandResponse *cr = g_malloc0(sizeof(struct CommandResponse));
    command_request_response_init((CommandRequestResponse*)cr);
    return cr;
}

CommandResponse *command_response_new2(const char *kind, const char *key)
{
    CommandResponse *cr = g_malloc0(sizeof(struct CommandResponse));
    command_request_response_init2((CommandRequestResponse*)cr, kind, key);
    return cr;
}

void command_request_response_destroy (CommandRequestResponse *cr);

void command_request_destroy (CommandRequest *cr)
{
    command_request_response_destroy((CommandRequestResponse*)cr);
    g_free(cr);
}

void command_response_destroy (CommandResponse *cr)
{
    command_request_response_destroy((CommandRequestResponse*)cr);
    g_free(cr);
}

void command_request_response_destroy (CommandRequestResponse *cr)
{
    while (lock_timed_out(command_lock(cr))) {}
    sem_destroy(&cr->lock);
    g_string_free(cr->buffer, TRUE);
    g_free((gpointer)cr->kind);
    g_free((gpointer)cr->key);
}

CommandManager *command_manager_new()
{
    CommandManager *cm = g_malloc0(sizeof(struct CommandManager));
    cm->requests = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, (GDestroyNotify)command_request_destroy);
    cm->responses = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, (GDestroyNotify)command_response_destroy);
    cm->command_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    cm->errors = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)g_error_free);
    return cm;
}

void command_manager_destroy(CommandManager *cm)
{
    g_hash_table_destroy(cm->command_table);
    g_hash_table_destroy(cm->responses);
    g_hash_table_destroy(cm->requests);
    g_hash_table_destroy(cm->errors);
    g_free(cm);
}

CommandManager *command_init()
{
    CommandManager *res = command_manager_new();
    return res;
}

void command_manager_handler_register(CommandManager *cm, const char *kind, command_do func)
{
    if (kind == NULL)
    {
        g_hash_table_insert(cm->command_table, g_strdup("default"), func);
    }
    else
    {
        g_hash_table_insert(cm->command_table, g_strdup(kind), func);
    }
}

command_do command_manager_get_handler(CommandManager *cm, const char *kind)
{
    return g_hash_table_lookup(cm->command_table, kind);
}

void command_manager_response_destroy (CommandManager *cm, const char *key)
{
    g_hash_table_remove(cm->responses, key);
}

void command_manager_request_destroy (CommandManager *cm, const char *key)
{
    g_hash_table_remove(cm->requests, key);
}

void command_manager_error_destroy (CommandManager *cm, const char *key)
{
    g_hash_table_remove(cm->errors, key);
}

ssize_t _command_write(CommandRequestResponse *resp, struct WriteParams wd)
{
    while (command_lock(resp) == -1) {}
    ssize_t res = _write(command_buffer(resp), wd);
    command_unlock(resp);
    return res;
}

ssize_t _command_read(CommandRequestResponse *resp, struct ReadParams rd)
{
    while (command_lock(resp) == -1) {}
    ssize_t res = _read(command_buffer(resp), rd);
    command_unlock(resp);
    return res;
}

size_t _command_size(CommandRequestResponse *req)
{
    while (command_lock(req) == -1) {}
    size_t res = command_buffer(req)->len;
    command_unlock(req);
    return res;
}

ssize_t _write(GString *str, struct WriteParams wd)
{
    if (str->len < wd.offset + wd.size)
    {
        size_t size = wd.offset + wd.size;
        size_t orig_len = str->len;
        g_string_set_size(str, size);
        memset(str->str + orig_len, 0, size - orig_len);
    }

    g_string_overwrite_len(str, wd.offset, wd.buf, wd.size);

    return wd.size;
}

ssize_t _read(GString *str, struct ReadParams rd)
{
    if (str->len <= rd.offset)
    {
        return 0;
    }
    else
    {
        size_t readcount = 0;
        if (rd.offset + rd.size <= str->len)
        {
            readcount = rd.size;
        }
        else
        {
            readcount = str->len - rd.offset;
        }
        memmove(rd.buf, str->str + rd.offset, readcount);
        return readcount;
    }
    return rd.size;
}

CommandRequest *command_manager_request_new(CommandManager *cm, const char *kind, const char *key)
{
    if (key == NULL)
    {
        return NULL;
    }

    if (kind == NULL)
    {
        kind = "default";
    }
    CommandRequest *req = command_request_new2(kind, key);
    g_hash_table_insert(cm->requests, (gpointer) command_key(req), req);
    return req;
}

CommandResponse *command_manager_response_new(CommandManager *cm, CommandRequest *req)
{
    CommandResponse *res = command_response_new2(command_kind(req),command_key(req));
    g_hash_table_insert(cm->responses, (gpointer) command_key(res), res);
    return res;
}

void command_manager_handle (CommandManager *cm, CommandRequest *req)
{
    GError *err = NULL;
    CommandResponse *resp = NULL;
    command_do handler = command_manager_get_handler(cm, command_kind(req));
    if (handler)
    {
        resp = command_response_new2(command_kind(req), command_key(req));
        gboolean handled = FALSE;
        while (!handled)
        {
            if (!command_lock(req))
            {
                handler(resp, req, &err);
                handled = TRUE;
                command_unlock(req);
            }
            else if (errno == ETIMEDOUT)
            {
                warn("Timeout waiting for request lock in command_manager_handle");
            }
        }
    }
    else
    {
        g_set_error(&err,
                TAGFS_COMMAND_ERROR,
                TAGFS_COMMAND_ERROR_NO_SUCH_COMMAND,
                "No handler for command kind '%s'", command_kind(req));
    }

    if (err)
    {
        g_hash_table_insert(cm->errors, g_strdup(command_key(req)), err);
        info("command error:%s:%s:%s", command_kind(req), command_key(req), err->message);
        command_response_destroy(resp);
    }
    else
    {
        g_hash_table_insert(cm->responses, (gpointer) command_key(resp), resp);
    }
    command_manager_request_destroy(cm, command_key(req));
}

CommandRequest* command_manager_get_request(CommandManager *cm, const char *key)
{
    return g_hash_table_lookup(cm->requests, key);
}

CommandResponse *command_manager_get_response(CommandManager *cm, const char *key)
{
    return g_hash_table_lookup(cm->responses, key);
}

GError *command_manager_get_error(CommandManager *cm, const char *key)
{
    return g_hash_table_lookup(cm->errors, key);
}

