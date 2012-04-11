#include "tokenizer.h"
#include "stream.h"

// collects characters from a stream,
// generally a file, and spits out words
// divided by tokens specified by the user

Tokenizer *tokenizer_new (GList *separators)
{
    Tokenizer *res = malloc(sizeof(Tokenizer));
    res->separators = separators;
    return res;
}

int tokenizer_set_file_stream (Tokenizer *tok, const char *filename)
{

    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("Error openinng file stream");
        return -1;
    }
    TokenizerStream *stream = tokenizer_stream_new(FILE_S, fp);
    tok->stream = stream;
    return 0;
}

int tokenizer_set_str_stream (Tokenizer *tok, char *string)
{
    tok->stream = tokenizer_stream_new(STR_S, string);
    return 0;
}

void tokenizer_destroy (Tokenizer *tok)
{
    tokenizer_stream_close(tok->stream);
    g_list_free(tok->separators);
    free(tok);
}

char *tokenizer_next (Tokenizer *tok, char *separator)
{
    // stream_getc()
    int c = tokenizer_stream_getc(tok->stream);
    if (tokenizer_stream_is_empty(tok->stream))
        return NULL;
    GString *accu = g_string_new("");
    while (!tokenizer_stream_is_empty(tok->stream)
            && g_list_index(tok->separators, GINT_TO_POINTER(c)) == -1)
    {
        accu = g_string_append_c(accu, c);
        c = tokenizer_stream_getc(tok->stream);
    }
    if (tokenizer_stream_is_empty(tok->stream))
    {
        *separator = -1;
    }
    else
    {
        *separator = c;
    }
    char *res = g_strdup(accu->str);
    g_string_free(accu, TRUE);
    return res;
}
