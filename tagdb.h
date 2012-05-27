#ifndef TAGDB_H
#define TAGDB_H
#include <glib.h>
#include "code_table.h"
#include "types.h"

struct tagdb
{
    GHashTable **tables;
    CodeTable *tag_codes;
    GHashTable *tag_types;
    const gchar *db_fname;
    const gchar *types_fname;
    int last_id;
};
#define FILE_TABLE 0
#define TAG_TABLE 1

typedef struct tagdb tagdb;

tagdb *newdb (const char *fname, const char *tags_name);
void tagdb_save (tagdb *db, const char* db_fname, const char *tag_types_fname);

GHashTable *tagdb_files (tagdb *db);
GHashTable *tagdb_tagged_files (tagdb *db);
GHashTable *tagdb_untagged_files (tagdb *db);
GHashTable *get_files_by_tag_list (tagdb *db, GList *tags);

GHashTable *tagdb_get_item (tagdb *db, int item_id, int table_id);
gpointer tagdb_get_sub (tagdb *db, int item_id, int sub_id, int table_id);
GHashTable *tagdb_get_table(tagdb *db, int table_id);

// File and tag table operations
int tagdb_insert_item (tagdb *db, gpointer item, GHashTable *data, int table_id);
void tagdb_insert_sub (tagdb *db, int item_id, int new_id, gpointer new_data, int table_id);
void tagdb_remove_item (tagdb *db, int item_id, int table_id);
void tagdb_remove_sub (tagdb *db, int item_id, int sub_id, int table_id);

// Tag code table accessor/mutators
int tagdb_get_tag_code (tagdb *db, const char *tag_name);
char *tagdb_get_tag_value (tagdb *db, int code);
void tagdb_change_tag_name (tagdb *db, char *old_name, char *new_name);

// Tag Type accessor/mutators
int tagdb_get_tag_type (tagdb *db, const char *tag_name);
int tagdb_get_tag_type_from_code (tagdb *db, int code);
void tagdb_set_tag_type (tagdb *db, const char *tag_name, int type);
void tagdb_set_tag_type_from_code (tagdb *db, int tag_code, int type);
void tagdb_remove_tag_type (tagdb *db, const char *tag_name);
void tagdb_remove_tag_type_from_code (tagdb *db, int tag_code);
// Returns all of the matching files as
// id=>tag_value pairs
GHashTable *tagdb_get_files_by_tag_value (tagdb *db, const char *tag, gpointer value, 
        GCompareFunc cmp, int inclusion_condition);
GList *tagdb_get_file_tag_list (tagdb *db, int item_id);

#endif /*TAGDB_H*/
