#ifndef STREAM_H
#define STREAM_H

#include <stdio.h>
#include <glib.h>

enum StreamType {
    FILE_S,
    STR_S
};

union Medium
{
    char *str;
    FILE *file;
};

typedef union Medium Medium;

struct TokenizerStream
{
    enum StreamType type;
    Medium med;
    int size;
    int position; // kept for string streams
};

typedef struct TokenizerStream TokenizerStream;

char tokenizer_stream_getc (TokenizerStream *s);
gboolean tokenizer_stream_is_empty (TokenizerStream *s);
TokenizerStream *tokenizer_stream_new (int type, gpointer m);

#endif /* TOKENIZER_STREAM_H */
