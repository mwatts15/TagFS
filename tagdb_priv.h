#ifndef TAGDB_PRIV_H
#define TAGDB_PRIV_H
#include "tokenizer.h"
void tags_from_file (TagDB *db, Tokenizer *tok);
void tags_to_file (TagDB *db, FILE *f);
void files_from_file (TagDB *db, Tokenizer *tok);
void files_to_file (TagDB *db, FILE *f);
void _remove_sub (TagDB *db, int item_id, int sub_id, int table_id);
void _insert_sub (TagDB *db, int item_id, int new_id, 
        gpointer new_data, int table_id);
void _new_sub (TagDB *db, int item_id, int new_id, 
        gpointer new_data, int table_id);
void _insert_item (TagDB *db, int item_id,
        GHashTable *data, int table_id);
GHashTable *_sub_table_new();
#endif /* TAGDB_PRIV_H */
