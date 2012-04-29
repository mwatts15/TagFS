#include "query.h"
#include "util.h"
#include <stdio.h>

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

void debug_query(tagdb *db, char *qstring)
{
    query_t *q = parse(qstring);
    query_info(q);
    result_t *res = NULL;
    gpointer r;
    int type;

    act(db, q, &r, &type);
    res = encapsulate(type, r);
    res_info(res);

    printf("\n");
    g_free(q);
    g_free(res);
}

void query_file_has_tags (gpointer filen, gpointer db)
{
    printf("FILE NAME: %d\n", GPOINTER_TO_INT(filen));
    gchar idstr[16];
    sprintf(idstr, "%d", GPOINTER_TO_INT(filen));
    gchar *str = g_strjoin(" ", "FILE HAS_TAGS", idstr, "tag048", "tag041", "tag012", NULL);
    debug_query((tagdb*) db, str);

    g_free(str);
}

void query_is_empty (gpointer tagname, gpointer db)
{
    printf("TAG NAME: %s\n", (gchar*) tagname);
    gchar *str = g_strjoin(" ", "TAG IS_EMPTY", (gchar*) tagname, NULL);
    debug_query((tagdb*) db, str);
    g_free(str);
}

void query_tspec (tagdb *db)
{
    debug_query(db, "TAG TSPEC ~tag048~tag008~tag012");
    debug_query(db, "TAG TSPEC /");
    debug_query(db, "TAG TSPEC \\");
    debug_query(db, "TAG TSPEC /lonny");
    debug_query(db, "TAG TSPEC /name=autorun.inf");
    debug_query(db, "TAG TSPEC /name=file001\\name=file002");
    debug_query(db, "TAG TSPEC /tag048/name");
}

void query_tag_create (tagdb *db)
{
    debug_query(db, "TAG CREATE cheese INT\n");
}

void query_tag_rename (tagdb *db)
{
    debug_query(db, "TAG RENAME cheese cheddar\n");
}

void query_file_create (tagdb *db)
{
    debug_query(db, "FILE CREATE name:newfile\n");
    debug_query(db, "FILE CREATE");
    debug_query(db, "FILE ADD_TAGS 26 name:sperm tag048:2 cheese:777");
    debug_query(db, "TAG TSPEC /");
    debug_query(db, "TAG TSPEC /name");
    debug_query(db, "TAG TSPEC /name=newfile");
}

int main ()
{
    tagdb *db = newdb("test.db", "test.types");
    GList *t1 = g_list_new("blue", "name", "tag014");
    GList *t2 = g_hash_table_get_keys(db->tables[FILE_TABLE]);
    g_list_foreach(t1, query_is_empty, db);
    g_list_foreach(t2, query_file_has_tags, db);
    g_list_free(t1);
    g_list_free(t2);
    query_tag_create(db);
    query_tag_rename(db);
    query_file_create(db);
    query_tspec(db);
    tagdb_save(db, "query.db", "query.types");
    return 0;
}
