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
    GList *quotes;
};

typedef struct Tokenizer Tokenizer;

Tokenizer *tokenizer_new0 (void);
Tokenizer *tokenizer_new (GList *separators);
Tokenizer *tokenizer_new_v (const char **separators);
Tokenizer *tokenizer_new2 (GList *separators, GList *quotes);
void tokenizer_set_separators (Tokenizer *tok, GList *separators);
void tokenizer_set_quotes (Tokenizer *tok, GList *quotes);
int tokenizer_set_file_stream (Tokenizer *tok, const char *filename);
int tokenizer_set_str_stream (Tokenizer *tok, char *string);
// returns a newly allocated token string
char *tokenizer_next (Tokenizer *tok, char **separator);
void tokenizer_destroy (Tokenizer *tok);

#endif /* TOKENIZER_H */
