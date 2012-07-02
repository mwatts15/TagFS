#include <string.h>
#include "scanner.h"
#include "log.h"

// collects characters from a stream,
// generally a file, and spits out words
// divided by separators specified by the user

static int _log_level = 1;

Scanner *scanner_new0 (void)
{
    Scanner *res = malloc(sizeof(Scanner));
    res->separators = NULL;
    res->quotes = NULL;
    return res;
}

Scanner *scanner_new_v (const char **separators)
{
    Scanner *res = scanner_new0();
    int i = 0;
    while (separators[i] != NULL)
    {
        res->separators = g_list_append(res->separators, separators[i]);
        i++;
    }
    return res;
}

Scanner *scanner_new (GList *separators)
{
    Scanner *res = scanner_new0();
    res->separators = separators;
    return res;
}

// quotes is a pair (start-quote, end-quote)
Scanner *scanner_new2 (GList *separators, GList *quotes)
{
    Scanner *res = scanner_new(separators);
    res->quotes = quotes;
    return res;
}

Scanner *scanner_new2_v (const char **separators, const char **quotes)
{
    Scanner *res = scanner_new_v(separators);
    int i = 0;
    while (quotes[i] != NULL)
    {
        res->quotes = g_list_append(res->quotes, g_strdup(quotes[i]));
        i++;
    }
    return res;
}

void scanner_set_separators (Scanner *scn, GList *separators)
{
    scn->separators = separators;
}

void scanner_set_quotes (Scanner *scn, GList *quotes)
{
    scn->quotes = quotes;
}

int scanner_set_file_stream (Scanner *scn, const char *filename)
{

    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("Error openinng file stream");
        return -1;
    }
    ScannerStream *stream = scanner_stream_new(FILE_S, fp);
    scn->stream = stream;
    return 0;
}

int scanner_set_str_stream (Scanner *scn, char *string)
{
    scn->stream = scanner_stream_new(STR_S, string);
    return 0;
}

void scanner_destroy (Scanner *scn)
{
    scanner_stream_close(scn->stream);
    g_list_free(scn->separators);
    g_list_free(scn->quotes);
    free(scn);
}

// reads the head of the tokenizer stream and returns
// the first string in strings which matches the head
char *_check_stream_head (Scanner *scn, GList *strings)
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
    if (max == 0) // means our only separator is a NULL byte
    {
        int c = scanner_stream_getc(scn->stream);
        if (c == '\0')
            return strings->data;
        else
        {
            scanner_stream_seek(scn->stream, -1, SEEK_CUR);
            return NULL;
        }
    }
    char buf[max+1];
    memset(buf, '\0', max+1);
    size_t true_size = scanner_stream_read(scn->stream, buf, max);
//    log_msg("_check_stream_head true_size = %d\n", true_size);
    it = strings;
    while (it != NULL)
    {
        char *sep = (char*) it->data;
        int sep_width = strlen(sep);
        if (sep_width == 0)
            sep_width++;
//        log_msg("buf='%s', sep='%s'\n", buf, sep);
        if (memcmp(buf, sep, sep_width) == 0)
        {
            /*
               seek back if necessary
             */
            scanner_stream_seek(scn->stream, sep_width - true_size, SEEK_CUR);
            return sep;
        }
        it = it->next;
    }
    scanner_stream_seek(scn->stream, -true_size, SEEK_CUR);
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
char *_check_quote (Scanner *scn, int *side)
{
    if (scn->quotes == NULL)
        return NULL;
//    log_msg("scanner_next checking quote characters\n");
    char *qstr = _check_stream_head(scn, scn->quotes);
    *side = g_list_index(scn->quotes, qstr);
    return qstr;
}

char *_check_separators (Scanner *scn)
{
    //printf("checking separators: ");
    return _check_stream_head(scn, scn->separators);
}

// skip n tokens
void scanner_skip (Scanner *scn, int n)
{
    int i = 0;
    char *s;
    while (i < n)
    {
        g_free(scanner_next(scn, &s));
    }
}

char *scanner_next (Scanner *scn, char **separator)
{
//    log_msg("Entering scanner_next\n");
    char c = 0;
    GString *accu = g_string_new("");
    char *sep = NULL;
    char *quot = NULL;
    int qside = -1;
//    log_msg("scanner_next entering while loop\n");
//    log_msg("scanner_next scn->stream = %p\n", scn->stream);
    while (!scanner_stream_is_empty(scn->stream))
    {
        quot = _check_quote(scn, &qside);
//        log_msg("quotizer_next, in while loop quot=%s\n", quot);
        if (quot != NULL && qside == 0)
        {
            scn->quotes = g_list_reverse(scn->quotes);
            /*
               grab all of the charachters up to the end quote
               advancing the pointer past the end quote
               but including neither of the quote characters
             */
            int depth = 1; // poor man's recursion
            while (!scanner_stream_is_empty(scn->stream))
            {
                quot = _check_quote(scn, &qside);
                if (qside == 1)
                {
                    depth++;
                    scanner_stream_seek(scn->stream, -strlen(quot), SEEK_CUR);
                }
                if (qside == 0)
                {
                    depth--;
                    if (depth == 0)
                    {
                        //printf("quoted string: '%s'\n", accu->str);
                        break;
                    }
                    scanner_stream_seek(scn->stream, -strlen(quot), SEEK_CUR);
                }
                c = scanner_stream_getc(scn->stream);
                //printf("c1 = '%c'\n", c);
                g_string_append_c(accu, c);
            }
            scn->quotes = g_list_reverse(scn->quotes);
        }
        sep = _check_separators(scn);
        if (sep != NULL)
        {
            break;
        }
        //printf("accu: '%s'\n", accu->str);
        c = scanner_stream_getc(scn->stream);
        //printf("c2 = '%c'\n", c);
        g_string_append_c(accu, c);
    }
    if (scanner_stream_is_empty(scn->stream))
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
//    log_msg("Leaving scanner_next with %s\n", res);
    return res;
}

void scanner_seek (Scanner *scn, off_t offset)
{
    scanner_stream_seek(scn->stream, offset, SEEK_SET);
}

char *scanner_read (Scanner *scn, size_t *offset)
{
    char *res = malloc(*offset);
    size_t real_size = scanner_stream_read(scn->stream, res, *offset);
    *offset = real_size;
    return res;
}
