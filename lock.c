#include <time.h>
#include <assert.h>
#include <string.h>
#include "log.h"
#include "lock.h"

int lock_acquire (lock_t* l, int timeout)
{
    int status = 0;
    debug("lock_acquire:attempting lock on %p", l);
    struct timespec ts;
    ts.tv_sec = timeout;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        char buf[64];
        strerror_r(errno, buf, 64);
        error("lock_acquire:clock_gettime: %s", buf);
        return -1;
    }

    while ((status = sem_timedwait(l, &ts)) == -1 && errno == EINTR)
        continue;
    if (status == -1)
    {
        if (errno == ETIMEDOUT)
        {
            warn("lock_acquire:timed out on %p", l);
            errno = ETIMEDOUT; // ensuring that the errno is set appropriately
            return -1;
        }
        else if (errno == EINVAL)
        {
            error("lock_acquire: Invalid lock or time specification");
            errno = EINVAL; // ensuring that the errno is set appropriately
            return -1;
        }
    }
    return 0;
}

int lock_release (lock_t* l)
{
    debug("lock_release:releasing lock on %p", l);
    int val = -1;
    sem_getvalue(l, &val);
    assert(val == 0);
    return sem_post(l);
}

