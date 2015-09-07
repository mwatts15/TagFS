#ifndef MESSAGE_DBUS_INTERNAL_H
#define MESSAGE_DBUS_INTERNAL_H
#define MESSAGE_DBUS_POOL_SIZE 64
#include <dbus/dbus.h>
#include <inttypes.h>
struct DBusData {
    const char *object_name;
    const char *interface_name;
    DBusConnection *dbus_conn;
    DBusMessage *messages[MESSAGE_DBUS_POOL_SIZE];
    uint64_t message_allocation_pool;
    int message_counter;
};


#endif /* MESSAGE_DBUS_INTERNAL_H */

