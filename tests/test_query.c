#include <stdio.h>
#include "query.h"
#include "util.h"
#include "test_util.h"
#include "types.h"


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

void query_meta_search (tagdb *db)
{
    debug_query(db, "META SEARCH @tagged");
    debug_query(db, "META SEARCH @untagged");
    debug_query(db, "META SEARCH @all");
    debug_query(db, "META SEARCH name=autorun.inf");
    debug_query(db, "META SEARCH name=file001 OR name=file002");
    debug_query(db, "META SEARCH @all AND tag048 AND name");
    debug_query(db, "META SEARCH @all AND stocking.jpg");
}

void query_file_search (tagdb *db)
{
    debug_query(db, "FILE SEARCH @tagged");
    debug_query(db, "FILE SEARCH @untagged");
    debug_query(db, "FILE SEARCH @all");
    debug_query(db, "FILE SEARCH name=autorun.inf");
    debug_query(db, "FILE SEARCH name=file001 OR name=file002");
    debug_query(db, "FILE SEARCH @all AND tag048 AND name");
    debug_query(db, "FILE SEARCH @all AND stocking.jpg");
}

void query_tag_create (tagdb *db)
{
    debug_query(db, "TAG CREATE cheese INT\n");
    debug_query(db, "TAG CREATE cows STRING\n");
    debug_query(db, "TAG CREATE tender STRING tag011:55 tag001:6\n");
    debug_query(db, "TAG CREATE self STRING self:55 tag001:6 tag012:99999\n");
}

void query_tag_rename (tagdb *db)
{
    debug_query(db, "TAG RENAME cheese cheddar\n");
}

void query_tag_add_tags (tagdb *db)
{
    debug_query(db, "TAG ADD_TAGS tag010 cheddar tag048\n");
}

void query_file_create (tagdb *db)
{
    debug_query(db, "FILE CREATE name:newfile\n");
    debug_query(db, "FILE CREATE");
    debug_query(db, "FILE CREATE name:newfile \"not a real tag\":12");
    debug_query(db, "FILE CREATE name"); // no-value
    debug_query(db, "FILE CREATE tag001:15 tag001:78"); // multiple assignment
}

void query_file_rename (tagdb *db)
{
    debug_query(db, "FILE LIST_TAGS 920");
    debug_query(db, "FILE RENAME 920 sperrys");
    debug_query(db, "FILE LIST_TAGS 920");
}

int main ()
{
    tagdb *db = newdb("test.db", "test.types");
    print_hash(tagdb_get_table(db, TAG_TABLE));
    /*
    query_tag_create(db);
    query_tag_rename(db);
    query_file_rename(db);
    query_file_create(db);
    query_tag_add_tags(db);
    query_file_search(db);
    */
    query_meta_search(db);
    tagdb_save(db, "query.db", "query.types");
    return 0;
}
