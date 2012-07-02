#ifndef SCANNER_H
#define SCANNER_H

#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include "stream.h"

struct Scanner
{
    ScannerStream *stream;
    GList *separators;
    GList *quotes;
    int max_sep_len;
    int max_quot_len;
};

typedef struct Scanner Scanner;

Scanner *scanner_new0 (void);
Scanner *scanner_new (GList *separators);
Scanner *scanner_new_v (const char **separators);
Scanner *scanner_new2 (GList *separators, GList *quotes);
Scanner *scanner_new2_v (const char **separators, const char **quotes);
void scanner_set_separators (Scanner *scn, GList *separators);
void scanner_set_quotes (Scanner *scn, GList *quotes);
int scanner_set_file_stream (Scanner *scn, const char *filename);
int scanner_set_str_stream (Scanner *scn, char *string);
// returns a newly allocated token string
char *scanner_next (Scanner *scn, char **separator);
void scanner_destroy (Scanner *scn);
void scanner_seek (Scanner *scn, off_t offset);
void scanner_skip (Scanner *scn, int n);

/* Reads up to n_bytes from stream returning
   a newly-allocated string */
char *scanner_read (Scanner *scn, size_t *n_bytes);

#endif /* SCANNER_H */
