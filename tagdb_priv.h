#ifndef TAGDB_PRIV_H
#define TAGDB_PRIV_H
void tag_types_from_file (tagdb *db, const char *types_fname);
void tag_types_to_file (tagdb *db, const char* filename);
void dbstruct_from_file (tagdb *db, const char *db_fname);
void dbstruct_to_file (tagdb *db, const char *filename);
void _remove_sub (tagdb *db, int item_id, int sub_id, int table_id);
void _insert_sub (tagdb *db, int item_id, int new_id, 
        gpointer new_data, int table_id);
void _insert_item (tagdb *db, int item_id,
        GHashTable *data, int table_id);
GHashTable *_sub_table_new();
#endif /* TAGDB_PRIV_H */
