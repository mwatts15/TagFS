#include "scanner.h"
#include "util.h"
#include "test_util.h"

void _test(char *test_name, Scanner *scn,
        char *infile, char *resfile, char *verifile)
{
    if (infile != NULL)
    {
        if (scanner_set_file_stream(scn, infile) == -1)
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
    while (!scanner_stream_is_empty(scn->stream))// && i < 55)
    {
        token = scanner_next(scn, &separator);
        //char *esc = g_strescape(separator, "");
        //printf("sep: '%s'\n", esc);
        //g_free(esc);
        //esc = NULL;
        fprintf(f, "%s\n%s\n", token, separator);
        g_free(token);
        i++;
    }
    fclose(f);
    scanner_destroy(scn);
    print_result(test_name, resfile, verifile);
}

void test_quotes()
{
    GList *seps = g_list_new("\n", " ", NULL);
    GList *quotes = g_list_new("{", "}", NULL);
    Scanner *scn = scanner_new2(seps, quotes);
    _test("quotes test", scn, "tq", "tq_out", "tq_ver");
}
void test_string()
{
    GList *seps = g_list_new("-", " ", NULL);
    GList *quotes = g_list_new("{", "}", NULL);
    Scanner *scn = scanner_new2(seps, quotes);
    scanner_set_str_stream(scn, "how do y{ou sleep at{ ni}ght you he}-athen beast?");
    _test("string test", scn, NULL, "ts_out", "ts_ver");
}
void test_varlen_seps()
{
    GList *seps = g_list_new("\n", " ", "--", "###", "=", "~~~", NULL);
    GList *quotes = g_list_new("%{", "}", NULL);
    Scanner *scn = scanner_new2(seps, quotes);
    _test("string test", scn, "tvl", "tvl_out", "tvl_ver");
}
void test_same_quote()
{
    GList *seps = g_list_new("\n", " ", "-", ":", NULL);
    GList *quotes = g_list_new("\"", NULL);
    Scanner *scn = scanner_new2(seps, quotes);
    _test("same quote test", scn, "sq", "sq_out", "sq_ver");
}
void test_vector_init()
{
    char *seps[5] = {"\n", " ", "-", ":", NULL};
    GList *quotes = g_list_new("\"", NULL);
    Scanner *scn = scanner_new_v(seps);
    scanner_set_quotes(scn, quotes);
    _test("same quote test", scn, "sq", "sq_out", "sq_ver");
}
int main (int argc, char **argv)
{
    test_quotes();
    test_string();
    test_varlen_seps();
    test_same_quote();
    test_vector_init();
    return 0;
}
