#include "query.h"
#include "util.h"
#include <stdio.h>

void query_info (query_t *q)
{
    if (q==NULL)
    {
        fprintf(stderr, "query_info: got q==NULL\n");
        return;
    }
    printf("query info:\n");
    printf("\ttable_id: %s\n", (q->table_id==FILE_TABLE)?"FILE_TABLE":"TAG_TABLE");
    printf("\tcommand: %s\n", q_commands[q->command_id]);
    printf("\targc: %d\n", q->argc);
    int i;
    for (i = 0; i < q->argc; i++)
    {
        printf("\targv[%d] = %s\n", i, q->argv[i]);
    }
}

void res_info (result_t *r)
{
    if (r == NULL)
    {
        return;
    }
    printf("result info:\n");
    printf("\ttype: %d\n", r->type);
    printf("\tdata: ");
    switch (r->type)
    {
        case tagdb_dict_t:
            print_hash(r->data.d);
            break;
        case tagdb_int_t:
            printf("%d\n", r->data.i);
            break;
        case tagdb_str_t:
            printf("%s\n", r->data.s);
            break;
        default:
            printf("%p\n", r->data.b);
    }
}

void query_file_has_tags (gpointer filen, gpointer db)
{
    printf("FILE NAME: %d\n", GPOINTER_TO_INT(filen));
    gchar idstr[16];
    sprintf(idstr, "%d", GPOINTER_TO_INT(filen));
    gchar *str = g_strjoin(" ", "FILE HAS_TAGS", idstr, "tag048", "tag041", "tag012", NULL);
    query_t *q = parse(str);
    query_info(q);
    gpointer r;
    result_t *res;
    int type = -1;
    act((tagdb*) db, q, &r, &type);
    res = encapsulate((tagdb*) db, type, r);
    res_info(res);
    printf("\n");

    g_free(str);
}

void query_is_empty (gpointer tagname, gpointer db)
{
    printf("TAG NAME: %s\n", (gchar*) tagname);
    gchar *str = g_strjoin(" ", "TAG IS_EMPTY", (gchar*) tagname, NULL);
    query_t *q = parse(str);
    query_info(q);
    gpointer r;
    result_t *res;
    int type = -1;
    act((tagdb*) db, q, &r, &type);
    res = encapsulate((tagdb*) db, type, r);
    res_info(res);
    printf("\n");

    g_free(str);
}

int main ()
{
    tagdb *db = newdb("test.db");
    GList *t1 = g_list_new("blue", "name", "tag014");
    GList *t2 = g_hash_table_get_keys(db->tables[FILE_TABLE]);
    g_list_foreach(t1, query_is_empty, db);
    g_list_foreach(t2, query_file_has_tags, db);
    g_list_free(t1);
    g_list_free(t2);
    return 0;
}
