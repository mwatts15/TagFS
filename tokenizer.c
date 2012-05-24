#include <string.h>
#include "tokenizer.h"

// collects characters from a stream,
// generally a file, and spits out words
// divided by separators specified by the user

Tokenizer *tokenizer_new0 (void)
{
    Tokenizer *res = malloc(sizeof(Tokenizer));
    res->separators = NULL;
    res->quotes = NULL;
    return res;
}

Tokenizer *tokenizer_new (GList *separators)
{
    Tokenizer *res = tokenizer_new0();
    res->separators = separators;
    return res;
}

// quotes is a pair (start-quote, end-quote)
Tokenizer *tokenizer_new2 (GList *separators, GList *quotes)
{
    Tokenizer *res = tokenizer_new(separators);
    res->quotes = quotes;
    return res;
}

void tokenizer_set_separators (Tokenizer *tok, GList *separators)
{
    tok->separators = separators;
}

void tokenizer_set_quotes (Tokenizer *tok, GList *quotes)
{
    tok->quotes = quotes;
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
    g_list_free(tok->quotes);
    free(tok);
}

// reads the head of the tokenizer stream and returns
// the first string in strings which matches the head
char *_check_stream_head (Tokenizer *tok, GList *strings)
{
    // get the maximum separator length
    GList *it = strings;
    int max = 0;
    while (it != NULL)
    {
        //printf("data: %p\n", it->data);
        int len = strlen((char*) it->data);
        if (max < len)
        {
            max = len;
        }
        it = it->next;
    }
    char buf[max+1];
    memset(buf, '\0', max+1);
    /*
       read and seek back so that if we don't
       find a separator, we are left with
       the stream as it was given to us
     */
    size_t true_size = tokenizer_stream_read(tok->stream, buf, max);
    tokenizer_stream_seek(tok->stream, -true_size, SEEK_CUR);
    // loop through the separators again checking 
    it = strings;
    while (it != NULL)
    {
        char *sep = (char*) it->data;
        //printf("buf='%s', sep='%s'\n", buf, sep);
        if (g_str_has_prefix(buf, sep))
        {
            /*
               advance the stream again to the
               end of the separator we retrieved
               spares users from doing it since
               it is the "common" usage 
             */
            tokenizer_stream_read(tok->stream, buf, strlen(sep));
            return sep;
        }
        it = it->next;
    }
    // against the read string
    return NULL;
}

// basically does the same thing
// as _check_separators but can
// be much simpler because we only
// allow one set of quote characters
// returns 
//  1 for the end quote
//  0 for the begin quote
// -1 if not a quote char
char *_check_quote (Tokenizer *tok, int *side)
{
    //printf("checking quotes: ");
    char *qstr = _check_stream_head(tok, tok->quotes);
    *side = g_list_index(tok->quotes, qstr);
    return qstr;
}

char *_check_separators (Tokenizer *tok)
{
    //printf("checking separators: ");
    return _check_stream_head(tok, tok->separators);
}

char *tokenizer_next (Tokenizer *tok, char **separator)
{
    char c = 0;
    GString *accu = g_string_new("");
    char *sep = NULL;
    char *quot = NULL;
    int qside = -1;
    while (!tokenizer_stream_is_empty(tok->stream))
    {
        quot = _check_quote(tok, &qside);
        if (quot != NULL && qside == 0)
        {
            tok->quotes = g_list_reverse(tok->quotes);
            /*
               grab all of the charachters up to the end quote
               advancing the pointer past the end quote
               but including neither of the quote characters
             */
            int depth = 1; // poor man's recursion
            while (!tokenizer_stream_is_empty(tok->stream))
            {
                quot = _check_quote(tok, &qside);
                if (qside == 1)
                {
                    depth++;
                    tokenizer_stream_seek(tok->stream, -strlen(quot), SEEK_CUR);
                }
                if (qside == 0)
                {
                    depth--;
                    if (depth == 0)
                    {
                        //printf("quoted string: '%s'\n", accu->str);
                        break;
                    }
                    tokenizer_stream_seek(tok->stream, -strlen(quot), SEEK_CUR);
                }
                c = tokenizer_stream_getc(tok->stream);
                //printf("c1 = '%c'\n", c);
                g_string_append_c(accu, c);
            }
            tok->quotes = g_list_reverse(tok->quotes);
        }
        sep = _check_separators(tok);
        if (sep != NULL)
        {
            break;
        }
        //printf("accu: '%s'\n", accu->str);
        c = tokenizer_stream_getc(tok->stream);
        //printf("c2 = '%c'\n", c);
        g_string_append_c(accu, c);
    }
    if (tokenizer_stream_is_empty(tok->stream))
    {
        *separator = "\377"; // end-of-file character
    }
    else
    {
        *separator = sep;
    }
    char *res = g_strdup(accu->str);
    g_string_free(accu, TRUE);
    //printf("\n");
    return res;
}
