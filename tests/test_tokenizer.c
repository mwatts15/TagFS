#include "tokenizer.h"
#include "util.h"
#include "test_util.h"

void _test(char *test_name, Tokenizer *tok,
        char *infile, char *resfile, char *verifile)
{
    if (infile != NULL)
    {
        if (tokenizer_set_file_stream(tok, infile) == -1)
            return;
    }

    FILE *f = fopen(resfile, "w");
    if (f == NULL)
    {
        perror("couldn't open resfile for read:");
        return;
    }

    char *separator = NULL;
    char *token = NULL;

    int i = 0;
    while (!tokenizer_stream_is_empty(tok->stream))// && i < 55)
    {
        token = tokenizer_next(tok, &separator);
        //char *esc = g_strescape(separator, "");
        //printf("sep: '%s'\n", esc);
        //g_free(esc);
        //esc = NULL;
        fprintf(f, "%s\n%s\n", token, separator);
        g_free(token);
        i++;
    }
    fclose(f);
    print_result(test_name, resfile, verifile);
}

void test_quotes()
{
    GList *seps = g_list_new("\n", " ", NULL);
    GList *quotes = g_list_new("{", "}", NULL);
    Tokenizer *tok = tokenizer_new2(seps, quotes);
    _test("quotes test", tok, "tq", "tq_out", "tq_ver");
}
void test_string()
{
    GList *seps = g_list_new("-", " ", NULL);
    GList *quotes = g_list_new("{", "}", NULL);
    Tokenizer *tok = tokenizer_new2(seps, quotes);
    tokenizer_set_str_stream(tok, "how do y{ou sleep at{ ni}ght you he}-athen beast?");
    _test("string test", tok, NULL, "ts_out", "ts_ver");
}
void test_varlen_seps()
{
    GList *seps = g_list_new("\n", " ", "--", "###", "=", "~~~", NULL);
    GList *quotes = g_list_new("%{", "}", NULL);
    Tokenizer *tok = tokenizer_new2(seps, quotes);
    _test("string test", tok, "tvl", "tvl_out", "tvl_ver");
}
void test_same_quote()
{
    GList *seps = g_list_new("\n", " ", "-", ":", NULL);
    GList *quotes = g_list_new("'", "'", NULL);
    Tokenizer *tok = tokenizer_new2(seps, quotes);
    _test("same quote test", tok, "sq", "sq_out", "sq_ver");
}
int main (int argc, char **argv)
{
    test_quotes();
    test_string();
    test_varlen_seps();
    test_same_quote();
    return 0;
}
