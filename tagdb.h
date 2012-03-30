#ifndef TAGDB_H
#define TAGDB_H
#include <glib.h>
#include "code_table.h"
#include "types.h"

struct tagdb
{
    GHashTable **tables;
    CodeTable *tag_codes;
    const gchar *db_fname;
    int last_id;
};
#define FILE_TABLE 0
#define TAG_TABLE 1

typedef struct tagdb tagdb;

tagdb *newdb (const char *fname);

GHashTable *tagdb_files (tagdb *db);
GHashTable *get_files_by_tag_list (tagdb *db, GList *tags);

result_t *tagdb_query (tagdb *db, const char *query);
GHashTable *tagdb_get_item (tagdb *db, int item_id, int table_id);
gpointer tagdb_get_sub (tagdb *db, int item_id, int sub_id, int table_id);
GHashTable *tagdb_get_table(tagdb *db, int table_id);
int tagdb_insert_item (tagdb *db, gpointer item, GHashTable *data, int table_id);
void tagdb_insert_sub (tagdb *db, int item_id, int new_id, gpointer new_data, int table_id);
void tagdb_remove_item (tagdb *db, int item_id, int table_id);
void tagdb_remove_sub (tagdb *db, int item_id, int sub_id, int table_id);
int tagdb_tag_code (tagdb *db, const char *tag_name);
int tagdb_get_tag_code (tagdb *db, const char *tag_name);
// Returns all of the matching files as
// id=>tag_value pairs
GHashTable *tagdb_get_files_by_tag_value (tagdb *db, const char *tag, gpointer value, 
        GCompareFunc cmp, int inclusion_condition);
#endif /*TAGDB_H*/
