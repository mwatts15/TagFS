#ifndef FILE_CABINET_H
#define FILE_CABINET_H
#include <glib.h>
#include "file_drawer.h"
#include "file_cabinet.h"
#include "file.h"

typedef GHashTable FileCabinet;

FileCabinet *file_cabinet_new ();
void file_cabinet_destroy (FileCabinet *fc);

/* Removes a file from a single slot */
void file_cabinet_remove (FileCabinet *fc, file_id_t slot_id, File *f);

/* Removes from all of the slots. All of them */
void file_cabinet_remove_v (FileCabinet *fc, tagdb_key_t slot_ids, File *f);

/* Inserts a file into a single slot */
void file_cabinet_insert (FileCabinet *fc, file_id_t slot_id, File *f);

/* Inserts into all of the slots. All of them */
void file_cabinet_insert_v (FileCabinet *fc, tagdb_key_t slot_ids, File *f);

void file_cabinet_remove_all (FileCabinet *fc, File *f);

void file_cabinet_new_drawer (FileCabinet *fc, file_id_t slot_id);

/* Returns the keyed file slot */
FileDrawer *file_cabinet_get_drawer (FileCabinet *fc, file_id_t slot_id);

/* Returns the keyed file slot as a GList */
GList *file_cabinet_get_drawer_l (FileCabinet *fc, file_id_t slot_id);

int file_cabinet_drawer_size (FileCabinet *fc, file_id_t key);

void file_cabinet_remove_drawer (FileCabinet *fc, file_id_t slot_id);
#endif /* FILE_CABINET_H */
