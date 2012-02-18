#include "tagdb.h"
#include <stdio.h>
#include <stdlib.h>

void print_list(GList *l)
{
    while (l != NULL)
    {
        printf("%s ", (char*) (l->data));
        l = g_list_next(l);
    }
    printf("\n");
}

gboolean print_node (GNode *node, gpointer not_used)
{
    printf("%*s%s\n", g_node_depth(node), " ", (char*) node->data);
    return FALSE;
}
void print_tree (GNode *tree)
{
    g_node_traverse(tree, G_PRE_ORDER, G_TRAVERSE_ALL, -1, print_node,
            NULL);
}
void print_pair (gpointer key, gpointer val, gpointer not_used)
{
    if (val == NULL)
    {
        val = "null";
    }
    printf("%s=>", (char*) key);
    printf("%p ", val);
}
void print_hash (GHashTable *hsh)
{
    g_hash_table_foreach(hsh, print_pair, NULL);
    printf("\n");
}
int main ()
{
    tagdb *db = newdb("test.db", "tags.list");
    int i;
    for (i = 0; i < 2000; i++)
    {
        printf("%d\n", i);
        get_files_by_tags(db, "tag1");
    }
}
