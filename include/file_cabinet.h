#ifndef FILE_CABINET_H
#define FILE_CABINET_H
#include <glib.h>
#include "sql.h"
#include "file.h"

typedef struct FileCabinet FileCabinet;

FileCabinet *file_cabinet_new0 (sqlite3 *db, GHashTable *files);
FileCabinet *file_cabinet_new (sqlite3 *db);
FileCabinet *file_cabinet_init (FileCabinet *res);
void file_cabinet_destroy (FileCabinet *fc);

/* Removes a file from a single slot */
void file_cabinet_remove (FileCabinet *fc, file_id_t slot_id, File *f);

/* Removes from all of the slots. All of them */
void file_cabinet_remove_v (FileCabinet *fc, tagdb_key_t slot_ids, File *f);

/* Inserts a file into a single slot */
void file_cabinet_insert (FileCabinet *fc, file_id_t slot_id, File *f);

/* Inserts a file into a single slot */
void file_cabinet_insert_with_value (FileCabinet *fc, file_id_t slot_id, File *f, gpointer val, size_t s);

/* Inserts into all of the slots. All of them */
void file_cabinet_insert_v (FileCabinet *fc, tagdb_key_t slot_ids, File *f);

void file_cabinet_remove_all (FileCabinet *fc, File *f);

/* Actually delete the file */
void file_cabinet_delete_file(FileCabinet *fc, File *f);

/* Returns the keyed file slot as a GList */
GList *file_cabinet_get_drawer_l (FileCabinet *fc, file_id_t slot_id);
GList *file_cabinet_get_drawer_tags (FileCabinet *fc, file_id_t slot_id);
tagdb_key_t file_cabinet_get_drawer_tags_a (FileCabinet *fc, file_id_t slot_id);
/* Returns files without any tags */
GList *file_cabinet_get_untagged_files (FileCabinet *fc);

int file_cabinet_drawer_size (FileCabinet *fc, file_id_t key);

void file_cabinet_remove_drawer (FileCabinet *fc, file_id_t slot_id);

/* returns the number of drawers */
gulong file_cabinet_size (FileCabinet *fc);

File *file_cabinet_lookup_file (FileCabinet *fc, tagdb_key_t tag_id, const char *name);

gboolean file_cabinet_has_untagged_files (FileCabinet *fc);

#endif /* FILE_CABINET_H */
