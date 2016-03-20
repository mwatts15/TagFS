#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <dbus/dbus.h>
#include "log.h"
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
#define _get_pool(__conn) (_get_data(__conn)->message_allocation_pool)
void mdbus_destroy_message (MessageConnection *conn, int message_index);
static void _destroy_message(MessageConnection *conn, int message_index);

int mdbus_prepare_signal (MessageConnection *conn, const char *signal)
{
    int idx = -1;
    int found = 0;
    int stat = lock_acquire(&_get_data(conn)->message_pool_lock, 1);
    if (stat == 0)
    {
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
            DBusMessage *msg = dbus_message_new_signal (_get_data(conn)->object_name,
                    _get_data(conn)->interface_name, signal);
            debug("new %p", msg);
            _get_data(conn)->messages[idx] = msg;
            debug("Setting %d", idx);
            _set_pool(conn, idx);
            debug(DBUS_MESSAGE_POOL_FMT , _get_pool(conn));
            _get_data(conn)->message_counter = (idx + 1) % MESSAGE_DBUS_POOL_SIZE;
        }
        else
        {
            idx = -1;
        }
        lock_release(&_get_data(conn)->message_pool_lock);
    }
    else
    {
        warn("Failed to acquire dbus message pool lock for message allocation");
    }
    return idx;
}

int mdbus_prepare_call (MessageConnection *conn, const char *receiver,
        const char *interface, const char *method)
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
    debug("ref %p", m);
    dbus_message_ref(m);
    _send(_get_data(conn)->dbus_conn, m);
}

int mdbus_send_signal (MessageConnection *conn, const char *signal)
{
    // XXX: Should we use the 'messages' cache here?
    int i = mdbus_prepare_signal(conn, signal);
    DBusMessage *msg = _get_message(conn, i);
    debug("ref %p", msg);
    dbus_message_ref(msg);
    _send(_get_data(conn)->dbus_conn, msg);
    mdbus_destroy_message(conn, i);
    return 0;
}

void mdbus_destroy_message (MessageConnection *conn, int message_index)
{
    int stat = lock_acquire(&_get_data(conn)->message_pool_lock, 1);
    if (stat == 0)
    {
        if ((message_index < MESSAGE_DBUS_POOL_SIZE) && _check_pool(conn, message_index))
        {
            _destroy_message(conn, message_index);
        } else {
            error("The given index %d doesn't correspond to a valid message", message_index);
        }
        lock_release(&_get_data(conn)->message_pool_lock);
    }
    else
    {
        warn("Failed to acquire dbus message pool lock for message destruction");
    }
}

static void _destroy_message(MessageConnection *conn, int message_index)
{
    DBusMessage *msg = _get_message(conn, message_index);
    debug("unref %p", msg);
    dbus_message_unref(msg);
    debug("Clearing %d", message_index);
    _clear_pool(conn, message_index);
    debug(DBUS_MESSAGE_POOL_FMT, _get_pool(conn));
}

int _send (DBusConnection *conn, DBusMessage *msg)
{
    dbus_connection_send(conn, msg, NULL);
    debug("unref %p", msg);
    dbus_message_unref(msg);
    return 0;
}

void mdbus_destroy (MessageConnection *conn)
{
    int stat = lock_acquire(&_get_data(conn)->message_pool_lock, 1);
    if (stat == 0)
    {
        for (int i = 0; i < MESSAGE_DBUS_POOL_SIZE; i++)
        {
            if (_check_pool(conn, i))
            {
                _destroy_message(conn, i);
            }
        }
    }
    sem_destroy(&_get_data(conn)->message_pool_lock);
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
    data->dbus_conn = dbus_bus_get(DBUS_BUS_SESSION, &error);
    data->message_allocation_pool = 0;
    data->message_counter = 0;
    data->interface_name = interface_name;
    sem_init(&data->message_pool_lock, 0, 1);

    res->user_data = data;
    res->sys = &ms;

    return res;
}
