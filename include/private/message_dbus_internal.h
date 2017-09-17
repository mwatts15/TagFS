#ifndef MESSAGE_DBUS_INTERNAL_H
#define MESSAGE_DBUS_INTERNAL_H
#define MESSAGE_DBUS_POOL_SIZE 64
#include <dbus/dbus.h>
#include <inttypes.h>
#include "lock.h"

struct DBusData {
    /** D-Bus object name */
    const char *object_name;
    /** D-Bus interface name */
    const char *interface_name;
    /** The connection to D-Bus */
    DBusConnection *dbus_conn;
    /** A pool of messages */
    DBusMessage *messages[MESSAGE_DBUS_POOL_SIZE];
    /** The index of allocated messages */
    uint64_t message_allocation_pool;
    /** An index one past the last allocated message */
    int message_counter;
    /** A read/write lock */
    lock_t message_pool_lock;
};

#define DBUS_MESSAGE_POOL_FMT "%016"PRIx64
void mdbus_destroy (MessageConnection *conn);

#endif /* MESSAGE_DBUS_INTERNAL_H */

