#ifndef MESSAGE_SYSTEM_H
#define MESSAGE_SYSTEM_H

typedef struct MessageSystem MessageSystem;
typedef struct {
    MessageSystem *sys;
    void *user_data;
} MessageConnection;


struct MessageSystem {
    /** Send a signal without any arguments */
    int (*send_signal) (MessageConnection *conn, const char *signal);

    /** Prepare a signal for sending later */
    int (*prepare_signal) (MessageConnection *conn, const char *signal);

    /** Prepare a call */
    int (*prepare_call) (MessageConnection *conn, const char *receiver,
            const char *interface, const char *method);

    /** Perform a method call */
    int (*call) (MessageConnection *conn, const char *target, int message_index,
            int timeoutInMS, void**result);

    /** Add an argument to a signal or a method */
    // XXX: Might distinguish between arguments to signals and methods later
    void (*add_arg) (MessageConnection *conn, int message_index,
            int tagdb_type, void *arg);

    /** Send a message previously prepared */
    void (*send) (MessageConnection *conn, int message_index);

    /** Destroy a message that has been prepared previously */
    void (*destroy_message)(MessageConnection *conn, int message_index);

    /** Destroy a message that has been prepared previously */
    void (*destroy) (MessageConnection *conn);
};

#define CALL(__conn, __method, ...) ((__conn)->sys->__method ((__conn), __VA_ARGS__))

#endif /* MESSAGE_SYSTEM_H */
