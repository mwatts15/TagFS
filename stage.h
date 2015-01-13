#ifndef STAGE_H
#define STAGE_H

#include "trie.h"
#include "tag.h"

typedef struct
{
    Trie *data;
    GHashTable *index;
} Stage;

Stage *new_stage ();

/* Adds the given name to the stage at the given
   posiiton */
void stage_add (Stage *s, tagdb_key_t position, AbstractFile *item);

/* Removes the given name from the given position */
void stage_remove (Stage *s, tagdb_key_t position, AbstractFile *f);

GList *stage_list_position (Stage *s, tagdb_key_t position);

AbstractFile* stage_lookup (Stage *s, tagdb_key_t position, file_id_t id);
void stage_remove_tag (Stage *s, AbstractFile *f);
void stage_destroy (Stage *s);

#endif /* STAGE_H */
