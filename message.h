#ifndef message_system_H
#define message_system_H

#include <dbus/dbus.h>

/** A MessageConnection refers to a single object in d-bus. The object implements various
 *  interfaces.
 */

typedef struct MessageConnection MessageConnection;
MessageConnection *message_system_init(char *object_name);
int message_system_send_signal (MessageConnection *conn, char *signal);
int message_system_prepare_signal (MessageConnection *conn, char *signal);
void message_system_add_arg (MessageConnection *conn, int message_index,
        int dbus_type, void *arg);
void message_system_send (MessageConnection *conn, int message_index);
void message_system_destroy_message(MessageConnection *conn, int message_index);
void message_system_destroy (MessageConnection *conn);

/* The number of messages that can be in use at one time */
#define MESSAGE_POOL_SIZE 64

#endif /* message_system_H */
