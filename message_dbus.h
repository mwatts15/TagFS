#ifndef MESSAGE_DBUS_H
#define MESSAGE_DBUS_H

#include <dbus/dbus.h>
MessageConnection *dbus_init (char *object_name);

#define MESSAGE_POOL_SIZE 64

#endif /* MESSAGE_DBUS_H */
