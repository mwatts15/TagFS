#ifndef MESSAGE_DBUS_H
#define MESSAGE_DBUS_H

#include "message.h"

MessageConnection *dbus_init (const char *object_name, const char *interface_name);

#endif /* MESSAGE_DBUS_H */
