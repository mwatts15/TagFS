#include <glib.h>
#include "code_table.h"
#ifndef TAGDB_H
#define TAGDB_H
struct tagdb
{
    GHashTable **tables;
    CodeTable *tag_codes;
    const gchar *db_fname;
};
#define FILE_TABLE 0
#define TAG_TABLE 1

typedef struct tagdb tagdb;

tagdb *newdb (const char *fname);

GHashTable *tagdb_files (tagdb *db);
GHashTable *get_files_by_tag_list (tagdb *db, GList *tags);

GHashTable *tagdb_get_item (tagdb *db, int item_id, int table_id);
GHashTable *tagdb_get_sub (tagdb *db, int item_id, int sub_id, int table_id);
GHashTable *tagdb_get_table(tagdb *db, int table_id);
void tagdb_insert_item (tagdb *db, int item_id, GHashTable *data, int table_id);
void tagdb_insert_sub (tagdb *db, int item_id, int new_id, gpointer new_data, int table_id);
void tagdb_remove_item (tagdb *db, int item_id, int table_id);
void tagdb_remove_sub (tagdb *db, int item_id, int sub_id, int table_id);
#endif /*TAGDB_H*/
