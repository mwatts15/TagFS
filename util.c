#include "util.h"
#include <string.h>

GList *pathToList (const char *path)
{
    if (g_str_has_prefix(path, "/"))
    {
        path++;
    }
    char **dirs = g_strsplit(path, "/", -1);
    GList *res = NULL;
    int i = 0;
    while (dirs[i] != NULL)
    {
        res = g_list_prepend(res, g_strdup(dirs[i]));
        i++;
    }
    g_strfreev(dirs);
    return g_list_reverse(res);
}

gboolean str_isalnum (const char *str)
{
    int n = strlen(str);
    int i;
    for (i = 0; i < n; i++)
    {
        if (!(g_ascii_isalnum(str[i]) ||
                str[i] == '-' ||
                str[i] == '_'))
        {
            return FALSE;
        }
    }
    return TRUE;
}

void print_list(FILE *out, GList *l)
{
    putc('(', out);
    while (l != NULL)
    {
        fprintf(out, "%d", *((int*) (l->data)));
        if (g_list_next(l) != NULL)
        {
            putc(' ', out);
        }
        l = g_list_next(l);
    }
    putc(')', out);
    putc('\n', out);
}

void print_pair (gpointer key, gpointer val, gpointer not_used)
{
    if (val == NULL)
    {
        val = "null";
    }
    printf("%p=>",  key);
    printf("%p,\n", val);
}

void print_hash (GHashTable *hsh)
{
    printf("{");
    g_hash_table_foreach(hsh, print_pair, NULL);
    printf("}");
    printf("\n");
}

