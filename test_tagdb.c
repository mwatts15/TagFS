#include "tagdb.h"
#include <stdio.h>
#include <stdlib.h>

void print_list(GList *l)
{
    while (l != NULL)
    {
        printf("%s ", l->data);
        l = g_list_next(l);
    }
    printf("\n");
}

gboolean print_node (GNode *node, gpointer not_used)
{
    printf("%*s%s\n", g_node_depth(node), " ", node->data);
    return FALSE;
}
void print_tree (GNode *tree)
{
    g_node_traverse(tree, G_PRE_ORDER, G_TRAVERSE_ALL, -1, print_node,
            NULL);
}
int main ()
{
    tagdb *db = newdb("test.db", "tags.list");
    /*
    tagdb *db = malloc(sizeof(tagdb));
    db->dbstruct = g_hash_table_new(NULL, g_str_equal);
    GNode *tree = g_node_new("");
    GNode *bar = g_node_append_data(tree, "tag1");
    GNode *foo = g_node_append_data(tree, "tag2");
    g_node_prepend_data(foo, "under_tag2");
    g_node_prepend_data(foo, "under_tag2_too");
    g_node_append_data(bar, "tagalicious");
    db->tagstruct = tree;

    

    GHashTable *tags = g_hash_table_new(NULL, g_str_equal);
    GHashTable *tags_too= g_hash_table_new(NULL, g_str_equal);
    GList *file_list;
    g_hash_table_insert(tags, "tag2", "");
    g_hash_table_insert(tags, "tag1", "");
    g_hash_table_insert(tags_too, "tag1", "");
    g_hash_table_insert(tags_too, "tag2", "");
    g_hash_table_insert(tags_too, "tagalicious", "");
    g_hash_table_insert(db->dbstruct, "omnitagged", tags);
    g_hash_table_insert(db->dbstruct, "thebitch", tags_too);
    print_list (get_tag_list(db));

    file_list = get_files_by_tags(db, "tag1", "tagalicious", "tag2", NULL);
    print_list(file_list);
    */
}
