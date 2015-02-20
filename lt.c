#include <sys/types.h>
#include <attr/xattr.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define BUFSIZE 10000
#define XATTR_PREFIX "user.tagfs."

static char buf[BUFSIZE];

int main (int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "lt expects a file name\n");
        return 1;
    }
    char *path = argv[1];
    ssize_t llength;
    int res = 0;
    llength = llistxattr(path, buf, BUFSIZE);
    int i = 0;
    if (llength >= 0)
    {
        char *p = buf;
        while (i < llength && p < buf + BUFSIZE)
        {

            if (strncmp(XATTR_PREFIX, p, strlen(XATTR_PREFIX)) == 0)
            {
                printf("%s\n", p + strlen(XATTR_PREFIX));
            }

            p = strchr(p, 0) + 1;
            i = p - buf;
        }
    }
    else
    {
        perror("lt");
        res = 1;
    }
    return res;
}
