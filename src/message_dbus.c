#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <dbus/dbus.h>
#include "types.h"
#include "message.h"
#include "message_dbus.h"
#include "message_dbus_internal.h"

static const int dbus_to_tagdb[] = {
    [TAGDB_DICT_TYPE] = DBUS_TYPE_INVALID,
    [TAGDB_LIST_TYPE] = DBUS_TYPE_INVALID,
    [TAGDB_INT_TYPE] = DBUS_TYPE_INT32,
    [TAGDB_STR_TYPE] = DBUS_TYPE_STRING,
    [TAGDB_BIN_TYPE] = DBUS_TYPE_INVALID
};

int _send (DBusConnection *, DBusMessage *);

#define _get_data(__conn) ((struct DBusData*)(__conn)->user_data)
#define _get_message(__conn, __msg_idx) ((_get_data(__conn))->messages[__msg_idx])
#define _set_pool(__conn, __msg_idx) (_get_data(__conn)->message_allocation_pool |= (1L << __msg_idx))
#define _clear_pool(__conn, __msg_idx) (_get_data(__conn)->message_allocation_pool &= ~(1L << __msg_idx))
#define _check_pool(__conn, __msg_idx) (_get_data(__conn)->message_allocation_pool & (1L << __msg_idx))
void mdbus_destroy_message (MessageConnection *conn, int message_index);

int mdbus_prepare_signal (MessageConnection *conn, char *signal)
{
    DBusMessage *msg = dbus_message_new_signal (_get_data(conn)->object_name,
            _get_data(conn)->interface_name, signal);
    int idx;
    int found = 0;
    for (int i = 0; i < MESSAGE_DBUS_POOL_SIZE; i++)
    {
        idx = (_get_data(conn)->message_counter + i) % MESSAGE_DBUS_POOL_SIZE;
        if (!_check_pool(conn, idx))
        {
            found = 1;
            break;
        }
    }
    if (found)
    {
        _get_data(conn)->messages[idx] = msg;
        _set_pool(conn, idx);
        _get_data(conn)->message_counter = (idx + 1) % MESSAGE_DBUS_POOL_SIZE;

        return idx;
    }
    else
    {
        dbus_message_unref(msg);
        return -1;
    }
}

int mdbus_prepare_call (MessageConnection *conn, char *receiver,
        char *interface, char *method)
{
    return 0;
}

void mdbus_add_arg (MessageConnection *conn, int message_index,
        int type, void *arg)
{
    DBusMessage *m = _get_message(conn, message_index);
    dbus_message_append_args(m, dbus_to_tagdb[type], arg, DBUS_TYPE_INVALID);
}

void mdbus_send (MessageConnection *conn, int message_index)
{
    DBusMessage *m = _get_message(conn, message_index);
    _send(_get_data(conn)->dbus_conn, m);
}

int mdbus_send_signal (MessageConnection *conn, char *signal)
{
    // XXX: Should we use the 'messages' cache here?
    int i = mdbus_prepare_signal(conn, signal);
    dbus_message_ref(_get_message(conn, i));
    _send(_get_data(conn)->dbus_conn, _get_message(conn, i));
    mdbus_destroy_message(conn, i);
    return 0;
}

void mdbus_destroy_message (MessageConnection *conn, int message_index)
{
    if ((message_index < MESSAGE_DBUS_POOL_SIZE) && _check_pool(conn, message_index))
    {
        dbus_message_unref(_get_message(conn, message_index));
        _clear_pool(conn, message_index);
    }
}

int _send (DBusConnection *conn, DBusMessage *msg)
{
    dbus_connection_send(conn, msg, NULL);
    dbus_message_unref(msg);
    return 0;
}

void mdbus_destroy (MessageConnection *conn)
{
    for (int i = 0; i < MESSAGE_DBUS_POOL_SIZE; i++)
    {
        mdbus_destroy_message(conn, i);
    }
    free((void*)_get_data(conn)->object_name);
    free(_get_data(conn));
    free(conn);
}

MessageSystem ms = {
    .send_signal = mdbus_send_signal,
    .prepare_signal = mdbus_prepare_signal,
    .prepare_call = mdbus_prepare_call,
    .add_arg = mdbus_add_arg,
    .send = mdbus_send,
    .destroy_message = mdbus_destroy_message,
    .destroy = mdbus_destroy,
};

MessageConnection *dbus_init (const char *object_name, const char *interface_name)
{
    DBusError error;
    MessageConnection *res = malloc(sizeof(MessageConnection));
    struct DBusData *data = malloc(sizeof(struct DBusData));
    dbus_error_init(&error);
    data->object_name = strdup(object_name);
    data->dbus_conn = dbus_bus_get (DBUS_BUS_SESSION, &error);
    data->message_allocation_pool = 0;
    data->message_counter = 0;
    data->interface_name = interface_name;

    res->user_data = data;
    res->sys = &ms;

    return res;
}
