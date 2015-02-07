#ifndef LOCK_H
#define LOCK_H
#include <errno.h>
#include <semaphore.h>
typedef sem_t lock_t;
/* Acquires the lock with a time-limit */
int lock_acquire(lock_t*, int timeout);
int lock_release(lock_t*);
#define lock_timed_out(__status) ((__status) == -1 && (errno == ETIMEDOUT))
#endif /* LOCK_H */

