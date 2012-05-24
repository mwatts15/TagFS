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
    long position; // kept for string streams
};

typedef struct TokenizerStream TokenizerStream;

char tokenizer_stream_getc (TokenizerStream *s);
// Only offset from the
int tokenizer_stream_seek (TokenizerStream *s, long offset, long origin);
gboolean tokenizer_stream_is_empty (TokenizerStream *s);
TokenizerStream *tokenizer_stream_new (int type, gpointer m);
int tokenizer_stream_close (TokenizerStream *stream);
size_t tokenizer_stream_read (TokenizerStream *s, char *buffer, size_t size);

#endif /* TOKENIZER_STREAM_H */
