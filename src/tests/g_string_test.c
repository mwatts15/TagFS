#include <glib.h>

int main()
{
    GString *s = g_string_new("");
    printf("s='%s'\n", s->str);
    int i;
    for (i = 0;i<19;i++)
        g_string_append_c(s, 'a');
    printf("s='%s'\n", s->str);
    return 0;
}
