#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "types.h"
#include "message.h"

struct MessageConnection {
    char *object_name;
    DBusConnection *dbus_conn;
    DBusMessage *messages[40];
    uint64_t message_allocation_pool;
    int message_counter;
};

static const int dbus_to_tagdb[] = {
    [TAGDB_DICT_TYPE] = DBUS_TYPE_INVALID,
    [TAGDB_LIST_TYPE] = DBUS_TYPE_INVALID,
    [TAGDB_INT_TYPE] = DBUS_TYPE_INT32,
    [TAGDB_STR_TYPE] = DBUS_TYPE_STRING,
    [TAGDB_BIN_TYPE] = DBUS_TYPE_INVALID
};

int _send (DBusConnection *, DBusMessage *);

#define _get_message(__conn, __msg_idx) __conn->messages[__msg_idx]

MessageConnection *message_system_init (char *object_name)
{
    DBusError error;
    MessageConnection *res = malloc(sizeof(MessageConnection));
    dbus_error_init(&error);
    res->object_name = strdup(object_name);
    res->dbus_conn = dbus_bus_get (DBUS_BUS_SESSION, &error);
    res->message_counter = 0;
    return res;
}

int message_system_prepare_signal (MessageConnection *conn, char *signal)
{
    DBusMessage *msg = dbus_message_new_signal (conn->object_name, "tagfs.fileEvents", signal);
    int idx = conn->message_counter;
    conn->messages[idx] = msg;
    conn->message_counter++;

    return idx;
}

void message_system_add_arg (MessageConnection *conn, int message_index,
        int type, void *arg)
{
    DBusMessage *m = _get_message(conn, message_index);
    dbus_message_append_args(m, dbus_to_tagdb[type], arg, DBUS_TYPE_INVALID);
}

void message_system_send (MessageConnection *conn, int message_index)
{
    DBusMessage *m = _get_message(conn, message_index);
    _send(conn->dbus_conn, m);
}

int message_system_send_signal (MessageConnection *conn, char *signal)
{
    // XXX: Should we use the 'messages' cache here?
    int i = message_system_prepare_signal(conn, signal);
    _send(conn->dbus_conn, conn->messages[i]);
    return 0;
}

int _send (DBusConnection *conn, DBusMessage *msg)
{
    dbus_connection_send(conn, msg, NULL);
    return 0;
}

void message_system_destroy (MessageConnection *conn)
{
    free(conn);
}
