#include "tokenizer.h"
// collects characters from a stream,
// generally a file, and spits out words
// divided by tokens specified by the user

Tokenizer *tokenizer_new (GList *separators)
{
    Tokenizer *res = malloc(sizeof(Tokenizer));
    res->separators = separators;
    return res;
}

void *tokenizer_destroy (Tokenizer *tok)
{
    fclose(tok->fp);
    g_list_free(tok->separators);
    free(tok);
}

int tokenizer_set_file_stream (Tokenizer *tok, const char *filename)
{
    tok->fp = fopen(filename, "r");
    if (tok->fp == NULL)
    {
        perror("Error openinng file stream");
        return -1;
    }
    return 0;
}

char *tokenizer_next (Tokenizer *tok, char *separator)
{
    // stream_getc()
    int c = fgetc(tok->fp);
    if (feof(tok->fp))
        return NULL;
    GString *accu = g_string_new("");
    while (!feof(tok->fp) 
            && g_list_index(tok->separators, GINT_TO_POINTER(c)) == -1)
    {
        accu = g_string_append_c(accu, c);
        c = fgetc(tok->fp);
    }
    if (feof(tok->fp))
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
