#ifndef MESSAGE_SYSTEM_H
#define MESSAGE_SYSTEM_H


/** A MessageConnection refers to a single object in d-bus. The object implements various
 *  interfaces.
 */

typedef struct MessageSystem MessageSystem;
typedef struct {
    MessageSystem *sys;
    void *user_data;
} MessageConnection;


struct MessageSystem {
    int (*send_signal) (MessageConnection *conn, char *signal);
    int (*prepare_signal) (MessageConnection *conn, char *signal);
    void (*add_arg) (MessageConnection *conn, int message_index,
            int dbus_type, void *arg);
    void (*send) (MessageConnection *conn, int message_index);
    void (*destroy_message)(MessageConnection *conn, int message_index);
    void (*destroy) (MessageConnection *conn);
};

#endif /* MESSAGE_SYSTEM_H */
