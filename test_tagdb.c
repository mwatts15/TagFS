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
    /*
    tagdb *db = newdb("test.db", "tags.list");
    print_hash(tagdb_get_file_tags(db, "file"));
    insert_file_tag(db, "file", "shabam!");
    print_hash(tagdb_get_file_tags(db, "file"));
    GNode *n = _path_to_node(tagdb_toTagTree(db), "/h/bc");
    printf("%p\n", n);
    n = _path_to_node(tagdb_toTagTree(db), "/h/pc");
    printf("%p\n", n);
    */
    GHashTable *hsh1 = g_hash_table_new(g_str_hash, g_str_equal);
    GHashTable *hsh2 = g_hash_table_new(g_str_hash, g_str_equal);
    GHashTable *tag1 = g_hash_table_new(g_str_hash, g_str_equal);
    GHashTable *tag2 = g_hash_table_new(g_str_hash, g_str_equal);

    g_hash_table_insert(tag1, "tag3", "");
    g_hash_table_insert(tag1, "tag1", "");
    g_hash_table_insert(tag1, "tag2", "");
    g_hash_table_insert(tag1, "tag4", "");

    g_hash_table_insert(tag2, "tag4", "");
    g_hash_table_insert(tag2, "tag2", "");
    g_hash_table_insert(tag2, "tag1", "");
    g_hash_table_insert(tag2, "tag3", "");


    printf("%d\n", file_tags_cmp(tag2, tag1));
}
