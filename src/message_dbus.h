#ifndef MESSAGE_DBUS_H
#define MESSAGE_DBUS_H

#include <dbus/dbus.h>
MessageConnection *dbus_init (char *object_name, char *interface_name);

#endif /* MESSAGE_DBUS_H */
