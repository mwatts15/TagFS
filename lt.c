#include <sys/types.h>
#include <attr/xattr.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "lt.h"

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
    int res = 0;
    XATTR_TAG_LIST(path, tag_name, res)
    {
        printf("%s\n", tag_name);
    } XATTR_TAG_LIST_END

    if (res)
    {
        perror("lt");
    }
    return res;
}
