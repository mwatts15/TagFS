#include <stdlib.h>
#include <string.h>
#include "stream.h"
#include "log.h"

#define stream_check( the_stream ) \
    if ( the_stream == NULL) \
        log_error("tokenizer_stream is NULL\n");

TokenizerStream *tokenizer_stream_new (int type, gpointer medium)
{
    TokenizerStream *s = malloc(sizeof(TokenizerStream));
    s->type = type;
    if (type == FILE_S)
        s->med.file = medium;
    if (type == STR_S)
    {
        s->position = 0;
        s->size = strlen(medium);
        s->med.str = g_strdup(medium);
    }
    return s;
}

// closes (and frees!) the given stream
int tokenizer_stream_close (TokenizerStream *stream)
{
    int stat = 0;
    if (stream->type == FILE_S)
    {
        stat = fclose(stream->med.file);
    }
    if (stream->type == STR_S)
    {
        g_free(stream->med.str);
    }
    g_free(stream);
    return stat;
}

char tokenizer_stream_getc (TokenizerStream *s)
{
    stream_check(s);
    if (s->type == FILE_S)
    {
        return fgetc(s->med.file);
    }
    if (s->type == STR_S)
    {
        return s->med.str[s->position++];
    }
    return 0;
}

// Note: this will advance the stream pointer
size_t tokenizer_stream_read (TokenizerStream *s, char *buffer, size_t size)
{
    stream_check(s);
    if (s->type == FILE_S)
    {
        return fread(buffer, 1, size, s->med.file);
    }
    if (s->type == STR_S)
    {
        size_t bytes_left = strlen(s->med.str) - s->position;
        size_t real_read = (bytes_left < size)? bytes_left : size;
        strncpy(buffer, s->med.str + s->position, real_read);
        s->position += real_read;
        return real_read;
    }
    return -1;
}

int tokenizer_stream_seek (TokenizerStream *s, long offset, long origin)
{
    stream_check(s);
    if (s->type == FILE_S)
    {
        fseek(s->med.file, offset, origin);
    }
    if (s->type == STR_S)
    {
        switch (origin)
        {
            case (SEEK_SET):
                origin = 0;
                break;
            case (SEEK_CUR):
                origin = s->position;
                break;
            case (SEEK_END):
                origin = strlen(s->med.str);
                break;
            default:
                origin = 0;
                break;
        }
        s->position = offset + origin;
        return 0;
    }
    return -1;
}

gboolean tokenizer_stream_is_empty (TokenizerStream *s)
{
    stream_check(s);
    if (s->type == FILE_S)
    {
        FILE *f = s->med.file;
        long pos = ftell(f);
        fseek(f, 0, SEEK_END);
        long end = ftell(f);
        fseek(f, pos, SEEK_SET);
        return (pos >= end);
    }
    if (s->type == STR_S)
    {
        return (s->position > s->size);
    }
    return TRUE;
}
