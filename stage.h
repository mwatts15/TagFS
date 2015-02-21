#ifndef STAGE_H
#define STAGE_H

#include "tag.h"
#include "key.h"

typedef struct
{
    GNode *tree;
    GHashTable *index;
} Stage;

Stage *new_stage ();

/* Adds the given name to the stage at the given
   posiiton */
gboolean stage_add (Stage *s, tagdb_key_t position, file_id_t item);

/* Removes the given name from the given position */
gboolean stage_remove (Stage *s, tagdb_key_t position, file_id_t f);

GList *stage_list_position (Stage *s, tagdb_key_t position);

gboolean stage_lookup (Stage *s, tagdb_key_t position, file_id_t id);
void stage_remove_all (Stage *s, file_id_t f);
void stage_destroy (Stage *s);

#endif /* STAGE_H */
