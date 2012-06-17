#ifndef TAGDB_UTIL_H
#define TAGDB_UTIL_H

#define KL(key, i) \
int i = 0; \
do { \

#define KL_END(key, i) i++; } while (key[i] != 0)
void print_key (gulong *k);

#endif /* TAGDB_UTIL_H */
