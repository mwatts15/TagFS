#ifndef LT_H
#define LT_H
#include <string.h>
/* This is meant to be an include for a tagfs utility */
#define BUFSIZE 10000
#define XATTR_PREFIX "user.tagfs."

static char __lt_buf[BUFSIZE];
#define XATTR_TAG_LIST(__path, __tag_name, __res) \
{ \
    ssize_t __llength;\
    int __res = 0;\
    int __i = 0;\
    __llength = llistxattr(__path, __lt_buf, BUFSIZE);\
    if (__llength < 0)\
    { \
        __res = 1;\
    } \
    else \
    {\
        char *__p = __lt_buf;\
        while (__i < __llength && __p < __lt_buf + BUFSIZE)\
        {\
            if (strncmp(XATTR_PREFIX, __p, strlen(XATTR_PREFIX)) == 0)\
            {\
                const char* __tag_name = __p + strlen(XATTR_PREFIX);\

#define XATTR_TAG_LIST_END \
            }\
            \
            __p = strchr(__p, 0) + 1;\
            __i = __p - __lt_buf;\
        }\
    }\
};\


#endif /* LT_H */

