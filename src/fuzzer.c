#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include "tagfs_common.h"
#include "params.h"

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

void rnd_finfo(struct drand48_data *rand_state, struct fuse_file_info *finfo)
{
    long s;
    lrand48_r(rand_state, &s);
    finfo->fh = s;
    lrand48_r(rand_state, &s);
    finfo->flags = (int)s;
}

int main (int argc, char **argv, char **envp)
{
    struct tagfs_state *data = process_options0(&argc, &argv, 0);
    fake_fuse_init(data);
    int res = 0;
    struct drand48_data rand_state;
    long seed = 0;
    const int numops = 1;
    int opcounts[numops];
    int opfreqs[] = {
        2
    };
    int iterations = 10000;
    if (argc >= 2)
    {
        int scanstat = sscanf(argv[1], "%ld", &seed);
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
    int srandstat = srand48_r(seed, &rand_state);
    if (srandstat != 0)
    {
        printf("The interface for `srand48_r' has changed or there was a serious error. Recieved an unexpected return value = %d\n", srandstat);
        res = -2;
        goto END;
    }

    int curit = 0;
    while (curit < iterations) {
        char cb01[1024];
        char cb02[1024];
        struct fuse_file_info finfo;
        rndpath(&rand_state, cb01, 1023);
        long moof;
        lrand48_r(&rand_state, &moof);
        mode_t rndmode = (mode_t)moof;

        long op = 0;
        lrand48_r(&rand_state, &op);
        if ((op % opfreqs[0]) == 0)
        {
            tagfs_operations_oper.create(cb01, rndmode, &finfo);
            opcounts[0]++;
        }
        curit++;
    }

    printf("\r%d", curit);
    for (int i = 0; i < numops; i++)
    {
        printf(" %d", opcounts[i]);
    }
    END:
    tagfs_operations_oper.destroy(data);
    return res;
}
