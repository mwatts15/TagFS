#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tagdb.h"
#include "command.h"
#include "util.h"

ssize_t _write(GString *str, struct WriteParams wd);
ssize_t _read(GString *str, struct ReadParams rd);
#define TAGFS_COMMAND_ERROR tagfs_command_error_quark ()

GQuark tagfs_command_error_quark ()
{
    return g_quark_from_static_string("tagfs-command-error-quark");
}

GString *command_output(CommandResponse *cr)
{
    return cr->result_buffer;
}

CommandRequest *command_request_new()
{
    CommandRequest *cr = g_malloc0(sizeof(struct CommandRequest));
    cr->command_buffer = g_string_new(NULL);
    return cr;
}

CommandRequest *command_request_new2(const char *kind, const char *key)
{
    CommandRequest *cr = command_request_new();
    cr->key = g_strdup(key);
    cr->kind = g_strdup(kind);
    return cr;
}

CommandResponse *command_response_new()
{
    CommandResponse *cr = g_malloc0(sizeof(struct CommandRequest));
    cr->result_buffer = g_string_new(NULL);
    return cr;
}

CommandResponse *command_response_new2(const char *kind, const char *key)
{
    CommandResponse *cr = command_response_new();
    cr->key = g_strdup(key);
    cr->kind = g_strdup(kind);
    return cr;
}

void command_request_destroy (CommandRequest *cr)
{
    g_string_free(cr->command_buffer, TRUE);
    g_free((gpointer)cr->kind);
    g_free((gpointer)cr->key);
    g_free(cr);
}

void command_response_destroy (CommandResponse *cr)
{
    g_string_free(cr->result_buffer, TRUE);
    g_free((gpointer)cr->kind);
    g_free((gpointer)cr->key);
    g_free(cr);
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

ssize_t command_write_response(CommandResponse *resp, struct WriteParams wd)
{
    return _write(resp->result_buffer, wd);
}

ssize_t command_read_response(CommandResponse *resp, struct ReadParams rd)
{
    return _read(resp->result_buffer, rd);
}

ssize_t command_write_request(CommandRequest *resp, struct WriteParams wd)
{
    return _write(resp->command_buffer, wd);
}

ssize_t command_read_request(CommandRequest *resp, struct ReadParams rd)
{
    return _read(resp->command_buffer, rd);
}

size_t command_request_size(CommandRequest *req)
{
    return req->command_buffer->len;
}

size_t command_response_size(CommandResponse *res)
{
    return res->result_buffer->len;
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
    printf("Inserting %p with key <%s>\n", req, key);
    g_hash_table_insert(cm->requests, (gpointer) req->key, req);
    return req;
}

CommandResponse *command_manager_response_new(CommandManager *cm, CommandRequest *req)
{
    CommandResponse *res = command_response_new2(req->kind, req->key);
    g_hash_table_insert(cm->responses, (gpointer) res->key, res);
    return res;
}

void command_manager_handle(CommandManager *cm, CommandRequest *req)
{
    GError *err = NULL;
    if (req)
    {
        CommandResponse *resp = command_manager_response_new(cm, req);
        command_do handler = command_manager_get_handler(cm, req->kind);
        handler(resp, req, &err);
        command_manager_request_destroy(cm, req->key);
    }
    else
    {
        g_set_error(&err,
                TAGFS_COMMAND_ERROR,
                TAGFS_COMMAND_ERROR_NO_SUCH_COMMAND,
                "Cannot handle a NULL request");
    }
    if (err)
    {
        g_hash_table_insert(cm->errors, g_strdup(req->key), err);
    }
}

void command_manager_handle_request(CommandManager *cm, const char *key)
{
    CommandRequest *req = command_manager_get_request(cm, key);
    command_manager_handle(cm, req);
}

CommandRequest* command_manager_get_request(CommandManager *cm, const char *key)
{
    return g_hash_table_lookup(cm->requests, key);
}

CommandResponse *command_manager_get_response(CommandManager *cm, const char *key)
{
    return g_hash_table_lookup(cm->responses, key);
}
