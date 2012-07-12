#ifndef TAGDB_UTIL_H
#define TAGDB_UTIL_H
#include "abstract_file.h"

typedef file_id_t *tagdb_key_t;

/* key for untagged files */
#define UNTAGGED 0ll

#define KL(key, i) \
{ \
    int i = 0; \
    for (i = 0; key[i] != 0; i++)

#define KL_END }
void print_key (tagdb_key_t k);

#endif /* TAGDB_UTIL_H */
