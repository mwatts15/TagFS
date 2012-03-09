#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include "stream.h"

struct Tokenizer
{
    TokenizerStream *stream;
    GList *separators;
};

typedef struct Tokenizer Tokenizer;

Tokenizer *tokenizer_new (GList *separators);
int tokenizer_set_file_stream (Tokenizer *tok, const char *filename);
int tokenizer_set_str_stream (Tokenizer *tok, char *string);
// returns a newly allocated token string
char *tokenizer_next (Tokenizer *tok, char *separator);

#endif /* TOKENIZER_H */
