#include <sys/types.h>
#include <attr/xattr.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "lt.h"
#define MAX_TAG_NAME_LENGTH 255

int main (int argc, char **argv)
{
    char *filename;
    char *tagname;
    char attr_name[MAX_TAG_NAME_LENGTH + 1];

    if (argc < 3)
    {
        fprintf(stderr, "ts expects a file name and a tag name\n");
        return 1;
    }
    filename = argv[1];
    tagname = argv[2];
    if (strlen(tagname) + strlen(XATTR_PREFIX) > MAX_TAG_NAME_LENGTH)
    {
        fprintf(stderr, "ts: The given tag name is too long");
        return 1;
    }
    strcpy(attr_name, XATTR_PREFIX);
    strcpy(attr_name+strlen(XATTR_PREFIX), tagname);
    int stat = lsetxattr(filename, attr_name, "", 0, 0);
    if (stat)
    {
        perror("ts");
        return 1;
    }
    return 0;
}
