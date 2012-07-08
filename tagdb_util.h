#ifndef TAGDB_UTIL_H
#define TAGDB_UTIL_H

#define KL(key, i) \
{ \
    int i = 0; \
    for (i = 0; key[i] != 0; i++)

#define KL_END }
void print_key (gulong *k);

#endif /* TAGDB_UTIL_H */
