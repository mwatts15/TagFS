#include "tokenizer.h"

int main (int argc, char **argv)
{
    GList *seps = NULL;
    seps = g_list_prepend(seps, GINT_TO_POINTER('\n'));
    seps = g_list_prepend(seps, GINT_TO_POINTER(' '));
    seps = g_list_prepend(seps, GINT_TO_POINTER(':'));
    seps = g_list_prepend(seps, GINT_TO_POINTER(','));
    seps = g_list_prepend(seps, GINT_TO_POINTER('='));
    Tokenizer *tok = tokenizer_new(seps);
    if (tokenizer_set_file_stream(tok, "test.db") == -1)
    {
        return -1;
    }
    char separator;
    char *token = tokenizer_next(tok, &separator);
    while (token != NULL)
    {
        printf("Got %s before %c\n", token, separator);
        g_free(token);
        token = tokenizer_next(tok, &separator);
    }
    tokenizer_set_str_stream(tok, ",shoop da:woop,woop=woop,blah\n");
    token = tokenizer_next(tok, &separator);
    while (token != NULL)
    {
        printf("Got %s before %c\n", token, separator);
        g_free(token);
        token = tokenizer_next(tok, &separator);
    }
    tokenizer_destroy(tok);
}
