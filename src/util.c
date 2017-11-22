#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "util.h"

#define ID_STRING_MAX_LEN 16
#define ID_TO_STRING(string_name, id_name) \
    char string_name[ID_STRING_MAX_LEN]; \
    g_snprintf(string_name, ID_STRING_MAX_LEN, "%d", id_name);
#define CMP(A, op, B, comparator) comparator(A, B) op 0

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

// va_list must of course be NULL-terminated
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
    va_end(args);
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
    va_end(args);
    return g_list_reverse(res);
}

/* lol must be a list of lists with no non-list members */
GList *g_list_flatten (GList *lol)
{
    GList *res = NULL;
    LL(lol, ita)
    {
        LL((GList*)ita->data, itb)
        {
            res = g_list_prepend(res, itb->data);
        }
    } LL_END;
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

void print_list (GList *l, ToString s)
{
    printf("(");
    while (l != NULL)
    {
        printf("%s", s(l->data));
        if (g_list_next(l) != NULL)
        {
            printf(" ");
        }
        l = g_list_next(l);
    }
    printf(")");
    printf("\n");
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
}

void fprint_pair (gpointer key, gpointer val, gpointer f)
{
    fprintf(f, "%p=>",  key);
    fprintf(f, "%p ", val);
}

void print_pair (gpointer key, gpointer val, gpointer not_used)
{
    printf("%p=>",  key);
    printf("%p ", val);
}

void fprint_hash (FILE *f, GHashTable *hsh)
{
    fprintf(f, "{");
    g_hash_table_foreach(hsh, fprint_pair, f);
    fprintf(f, "}");
    fprintf(f, "\n");
}

void print_hash (GHashTable *hsh)
{
    printf("{");
    if (hsh != NULL)
        g_hash_table_foreach(hsh, print_pair, NULL);
    printf("}");
    printf("\n");
}

void print_tree(GTree *tree)
{
    g_tree_foreach(tree,(GTraverseFunc) print_pair, NULL);
    printf("\n");
}

int max (int a, int b)
{
    return (a > b)?a:b;
}

/* for use with qsort */
int cmp (gconstpointer a, gconstpointer b)
{
    return *((gulong*) a) - *((gulong*) b);
}

int long_cmp (gpointer a, gpointer b)
{
    return TO_S(a) - TO_S(b);
}

// gets the index of a string in a null-terminated string vector
int strv_index (const char *vector[], const char *str)
{
    int i = 0;
    while (vector[i] != NULL)
    {
        if (str_equal(vector[i], str))
            return i;
        i++;
    }
    return -1;
}

char *to_charp (size_t size, size_t n)
{
    char *res = g_malloc0(size);
    int i;
    size_t mask =  SCHAR_MAX;
    for (i = 0; i < size; i++)
    {
        ldiv_t this = ldiv(n, mask);
        res[i] = this.rem;
        printf("%zd\n", this.rem);
        n = this.quot;
    }
    return res;
}

size_t charp_to_size (size_t length, char *s)
{
    size_t res = 0;
    size_t mask = SCHAR_MAX;
    size_t shift = 1;
    int i;

    for (i = 0; i < length; i++)
    {
        res += s[i] * shift;
        shift *= mask;
    }
    return res;
}

buffer_t new_buffer(size_t size)
{
    buffer_t b;
    b.size = size;
    b.content = calloc(1,size);
    return b;
}

buffer_t buffer_wrap(size_t size, char *content)
{
    buffer_t b;
    b.size = size;
    b.content = content;
    return b;
}

unsigned short rand_lim (unsigned short limit)
{
/* return a random number between 0 and limit inclusive.
 */
    int divisor = RAND_MAX/(limit+1);
    unsigned short retval;

    do {
        retval = rand() / divisor;
    } while (retval > limit);

    return retval + 1;
}

/* Copied from http://stackoverflow.com/a/2180788/638671 . Thanks to user 'caf'. */
int cp (const char *from, const char *to)
{
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
        return -1;

    fd_to = open(to, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd_to < 0)
        goto out_error;

    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR)
            {
                goto out_error;
            }
        } while (nread > 0);
    }

    if (nread == 0)
    {
        if (close(fd_to) < 0)
        {
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);

        /* Success! */
        return 0;
    }

  out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    errno = saved_errno;
    return -1;
}
