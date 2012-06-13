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

struct ScannerStream
{
    enum StreamType type;
    Medium med;
    int size;
    long position; // kept for string streams
};

typedef struct ScannerStream ScannerStream;

char scanner_stream_getc (ScannerStream *s);
// Only offset from the
int scanner_stream_seek (ScannerStream *s, long offset, long origin);
gboolean scanner_stream_is_empty (ScannerStream *s);
ScannerStream *scanner_stream_new (int type, gpointer m);
int scanner_stream_close (ScannerStream *stream);
size_t scanner_stream_read (ScannerStream *s, char *buffer, size_t size);

#endif /* STREAM_H */
