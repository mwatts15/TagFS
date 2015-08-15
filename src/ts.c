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
    char *tagname_start;
    size_t prefix_length = strlen(XATTR_PREFIX);
    int retstat = 0;

    if (argc < 3)
    {
        fprintf(stderr, "ts expects a file name and a tag name\n");
        return 1;
    }

    filename = argv[1];
    strcpy(attr_name, XATTR_PREFIX);
    tagname_start = attr_name + prefix_length;

    int arg_index=2;
    while (arg_index < argc)
    {
        tagname = argv[arg_index];
        if (strlen(tagname) + prefix_length > MAX_TAG_NAME_LENGTH)
        {
            fprintf(stderr, "ts: The tag name (%s) is too long", tagname);
            retstat = 1;
        }
        else
        {

            strcpy(tagname_start, tagname);
            int stat = lsetxattr(filename, attr_name, "", 0, 0);
            if (stat)
            {
                perror("ts");
                retstat = 1;
            }
        }

        arg_index++;
    }
    return retstat;
}
