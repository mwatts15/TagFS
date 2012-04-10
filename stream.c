#include <stdlib.h>
#include <string.h>
#include "stream.h"

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
        s->med.str = medium;
    }
    return s;
}

// closes (and frees!) the given stream
void tokenizer_stream_close (TokenizerStream *stream)
{
    if (stream->type == FILE_S)
    {
        int stat = fclose(stream->med.file);
    }
    if (stream->type == STR_S)
    {
        g_free(stream->med.str);
    }
    g_free(stream);
}

char tokenizer_stream_getc (TokenizerStream *s)
{
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

gboolean tokenizer_stream_is_empty (TokenizerStream *s)
{
    if (s->type == STR_S)
    {
        return (s->position > s->size);
    }
    if (s->type == FILE_S)
    {
        return feof(s->med.file);
    }
    return TRUE;
}
