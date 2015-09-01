#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "params.h"
#include "tagdb.h"
#include "command.h"
#include "util.h"

ssize_t _write(GString *str, const char *buf, size_t size, size_t offset);
ssize_t _read(GString *str, char *buf, size_t size, size_t offset);

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

CommandResponse *command_response_new()
{
    CommandResponse *cr = g_malloc0(sizeof(struct CommandRequest));
    cr->result_buffer = g_string_new(NULL);
    return cr;
}

void command_request_destroy (CommandRequest *cr)
{
    g_string_free(cr->command_buffer, TRUE);
    g_free(cr);
}

void command_response_destroy (CommandResponse *cr)
{
    g_string_free(cr->result_buffer, TRUE);
    g_free(cr);
}

CommandManager *command_mannager_new()
{
    CommandManager *cm = g_malloc0(sizeof(struct CommandManager));
    cm->requests = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, (GDestroyNotify)command_request_destroy);
    cm->responses = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, (GDestroyNotify)command_response_destroy);
    cm->command_table = g_hash_table_new(g_str_hash, g_str_equal);
    return cm;
}

ssize_t command_write_response(CommandResponse *resp, const char *buf, size_t size, size_t offset)
{
    return _write(resp->result_buffer, buf, size, offset);
}

ssize_t command_read_response(CommandResponse *resp, char *buf, size_t size, size_t offset)
{
    return _read(resp->result_buffer, buf, size, offset);
}

ssize_t command_write_request(CommandRequest *resp, const char *buf, size_t size, size_t offset)
{
    return _write(resp->command_buffer, buf, size, offset);
}

ssize_t command_read_request(CommandRequest *resp, char *buf, size_t size, size_t offset)
{
    return _read(resp->command_buffer, buf, size, offset);
}

ssize_t _write(GString *str, const char *buf, size_t size, size_t offset)
{
    g_string_overwrite_len(str, offset, buf, size);
    return size;
}

ssize_t _read(GString *str, char *buf, size_t size, size_t offset)
{
    if (str->len == offset)
    {
        return 0;
    }
    else if (str->len < offset)
    {
        size_t readcount = 0;
        if (str->len <= offset + size)
        {
            readcount = size;
        }
        else
        {
            readcount = str->len - offset;
        }
        memmove(buf, str->str + offset, readcount);
        return size;
    }
    else
    {
        return -1;
    }
    return size;
}
