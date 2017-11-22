#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include "tagfs_common.h"
#include "params.h"
#include "util.h"

#define NUMOPS 2
#define MAXTHREADS 64
#define MAX_ERR_MSG_SIZE 512
int iterations = 10000;
long g_seed = 0;
int opcounts[NUMOPS] = {0};
int opfreqs[NUMOPS] = {
    2,
    3
};
int g_nthreads = 4;
pthread_t threads[MAXTHREADS];
sem_t t_complete_notice;

void rndpath(struct drand48_data *rand_state, char *cb01, int max)
{
    const int lmax = 1024;
    if (max > lmax - 1)
    {
        max = lmax - 1;
    }
    long smaz = 0;
    long smoz = 0;
    long frob = 0;
    cb01[0] = '/';
    lrand48_r(rand_state, &frob);

    int i;
    for (i = 1; i < (frob % max); i++)
    {
        lrand48_r(rand_state, &smoz);
        char a = '/';
        if ((smoz % 50) != 0)
        {
            mrand48_r(rand_state, &smaz);
            a = (((unsigned char)smaz) % 26) + 'a' + ((smaz > 0) ? ('A' - 'a') : 0);
        }
        cb01[i] = a;
    }
    cb01[i] = 0;
}

void rndfifo(struct drand48_data *rand_state, struct fuse_file_info *finfo)
{
    long s;
    lrand48_r(rand_state, &s);
    finfo->fh = s;
    lrand48_r(rand_state, &s);
    finfo->flags = (int)s;
}

int run (long seed)
{
    int res = 0;
    struct drand48_data rand_state;
    int srandstat = srand48_r(seed, &rand_state);
    if (srandstat != 0)
    {
        fprintf(stderr, "The interface for `srand48_r' has changed or there was"
                " a serious error. Recieved an unexpected return value = %d\n",
                srandstat);
        res = -2;
        goto END;
    }

    char cb01[1024];
    char cb02[1024];
    char temp[1024];
    char cb02_l[1024];

    // init cb02_l in case it's needed before the first usage
    cb02_l[0] = 0;

    long op = 0;
    long moof;
    struct fuse_file_info finfo;
    for (int curit = 0; curit < iterations; curit++)
    {
        rndfifo(&rand_state, &finfo);
        rndpath(&rand_state, cb01, 1023);
        rndpath(&rand_state, cb02, 1023);
        lrand48_r(&rand_state, &moof);
        mode_t rndmode = (mode_t)moof;

        lrand48_r(&rand_state, &op);
        int actops = 0;
        for (int i = 0; i < NUMOPS; i++)
        {
            actops |= (((op % opfreqs[i]) == 0) << i);
        }

        if (actops & 1)
        {
            lrand48_r(&rand_state, &op);
            char *pth;
            if (op % 2 == 0)
            {
                memmove(temp, cb02_l, 1024);
                strncat(temp, cb01, 1023 - strlen(cb02_l));
                pth = temp;
            }
            else
            {
                pth = cb01;
            }
            tagfs_operations_oper.create(pth, rndmode, &finfo);
        }

        if (actops & 2)
        {
            tagfs_operations_oper.mkdir(cb02, rndmode);
            memmove(cb02_l, cb02, 1024);
        }

        for (int i = 0; i < NUMOPS; i++)
        {
            opcounts[i] += (actops & (1 << i)) >> i;
        }
    }

    END:
    return res;
}

void *run_thread_wrapper(void *arg)
{
    void *res = (void *) (long) run((long) arg);
    sem_post(&t_complete_notice);
    return res;
}

int main (int argc, char **argv, char **envp)
{
    struct tagfs_state *data = process_options0(&argc, &argv, 0);
    struct drand48_data rand_state;
    char errmsg[MAX_ERR_MSG_SIZE];
    fake_fuse_init(data);
    int res = 0;
    if (argc >= 2)
    {
        int scanstat = sscanf(argv[1], "%ld", &g_seed);
        if (scanstat != 1)
        {
            printf("The given argument %s is not a number\n", argv[1]);
            res = -1;
            goto END;
        }
    }
    if (argc >= 3)
    {
        int scanstat = sscanf(argv[2], "%d", &iterations);
        if (scanstat != 1)
        {
            printf("The given argument %s is not a number\n", argv[2]);
            res = -3;
            goto END;
        }
    }
    if (argc >= 4)
    {
        int scanstat = sscanf(argv[3], "%d", &g_nthreads);
        if (scanstat != 1)
        {
            printf("The given argument %s is not a number\n", argv[3]);
            res = -4;
            goto END;
        }
    }

    int srandstat = srand48_r(g_seed, &rand_state);
    if (srandstat != 0)
    {
        printf("The interface for `srand48_r' has changed or there was a"
               " serious error. Recieved an unexpected return value = %d\n",
               srandstat);
        res = -2;
        goto END;
    }

    pthread_attr_t attr;
    long seed;
    int n_born_threads = 0;
    int last = 0;
    for (int i = 0; !last && i < g_nthreads; i++)
    {
        lrand48_r(&rand_state, &seed);
        pthread_attr_init(&attr);
        int tstat = pthread_create(&threads[i], &attr,
                                   run_thread_wrapper,
                                   (void *)seed);
        if (tstat)
        {
            last = 1;
        }
        pthread_attr_destroy(&attr);

        n_born_threads += 1 - last;
    }

    int n_dead_threads = 0;
    while (n_born_threads > n_dead_threads)
    {
        sem_wait(&t_complete_notice);
        n_dead_threads++;
    }

    for (int i = 0; i < n_born_threads; i++)
    {
        void *tres;
        int joinstat = pthread_join(threads[i], &tres);
        if (joinstat)
        {
            strerror_r(joinstat, errmsg, MAX_ERR_MSG_SIZE);
            fprintf(stderr, "Error joining thread #%d: %s\n",
                    i, errmsg);
            abort();
        }
        if (tres)
        {
            fprintf(stderr, "Got an error return value (%d) from thread #%d\n",
                    (int) (long) tres, i);
        }
    }

    END:
    tagfs_operations_oper.destroy(data);
    return res;
}
