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

// because why didn't they make this standard!!!?
GList *g_list_new (gpointer first, ...)
{
    va_list args;
    GList *res = NULL;
    res = g_list_prepend(res, first);
    va_start(args, first);
    gpointer item = va_arg(args, gpointer);
    while (item != NULL)
    {
        res = g_list_prepend(res, item);
        item = va_arg(args, gpointer);
    }
    return g_list_reverse(res);
}

GList *g_list_new_charlist (gchar first, ...)
{
    va_list args;
    GList *res = NULL;
    res = g_list_prepend(res, GINT_TO_POINTER(first));
    va_start(args, first);
    int item = va_arg(args, int);
    while (item != 0)
    {
        res = g_list_prepend(res, GINT_TO_POINTER(item));
        item = va_arg(args, int);
    }
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
        fprintf(out, "%p", l->data);
        if (g_list_next(l) != NULL)
        {
            putc(' ', out);
        }
        l = g_list_next(l);
    }
    putc(')', out);
    putc('\n', out);
}

void print_string_list (GList *l)
{
    putc('(', stdout);
    while (l != NULL)
    {
        fprintf(stdout, "%s", (char*) l->data);
        if (g_list_next(l) != NULL)
        {
            putc(' ', stdout);
        }
        l = g_list_next(l);
    }
    putc(')', stdout);
    putc('\n', stdout);
}

void print_pair (gpointer key, gpointer val, gpointer not_used)
{
    printf("%p=>",  key);
    printf("%p ", val);
}

void print_hash (GHashTable *hsh)
{
    printf("{");
    g_hash_table_foreach(hsh, print_pair, NULL);
    printf("}");
    printf("\n");
}

void print_tree(GTree *tree)
{
    g_tree_foreach(tree,(GTraverseFunc) print_pair, NULL);
    printf("\n");
}
