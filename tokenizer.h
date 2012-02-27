#ifndef TOKENIZER_H
#define TOKENIZER_H
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>

struct Tokenizer
{
    FILE *fp;
    GList *separators;
};

typedef struct Tokenizer Tokenizer;

Tokenizer *tokenizer_new (GList *separators);
int tokenizer_set_file_stream (Tokenizer *tok, const char *filename);
// returns a newly allocated token string
char *tokenizer_next (Tokenizer *tok, char *separator);

#endif /* TOKENIZER_H */
