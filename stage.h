#ifndef STAGE_H
#define STAGE_H

#include "key.h"
#include "util.h"
#include "abstract_file.h"

typedef struct
{
    GHashTable *data;
} Stage;

Stage *new_stage ();
void stage_destroy (Stage *s);

/* Adds the given name to the stage at the given
   posiiton */
void stage_add (Stage *s, tagdb_key_t position, char *name, AbstractFile* item);

/* Removes the given name from the given position */
void stage_remove (Stage *s, tagdb_key_t position, char *name);

GList *stage_list_position (Stage *s, tagdb_key_t position);

AbstractFile* stage_lookup (Stage *s, tagdb_key_t position, char *name);

#endif /* STAGE_H */
