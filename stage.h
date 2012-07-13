#ifndef STAGE_H
#define STAGE_H

#include "trie.h"

typedef struct
{
    Trie *data;
} Stage;

Stage *new_stage ();

/* Adds the given name to the stage at the given
   posiiton */
void stage_add (Stage *s, trie_key_t position, char *name, gpointer item);

/* Removes the given name from the given position */
void stage_remove (Stage *s, trie_key_t position, char *name);

GList *stage_list_position (Stage *s, trie_key_t position);

int stage_lookup (Stage *s, trie_key_t position, char *name);

#endif /* STAGE_H */