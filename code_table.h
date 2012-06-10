#ifndef CODE_TABLE_H
#define CODE_TABLE_H
#include <glib.h>
#include <stdlib.h>
// associates numerical ids and strings
// numerical ids are stored as pointers using GINT_TO_POINTER
// in a forward and a reverse Hash Table
// The mappings are 1-to-1 (or should be) and you access by
// the rhs or lhs
struct CodeTable
{
    GHashTable *forward;
    GHashTable *reverse;
    gulong size; // keeps the size (number of valid entries)
    gulong last_id; // for choosing new entries.
};

typedef struct CodeTable CodeTable;

CodeTable *code_table_new();
gulong code_table_get_code (CodeTable *ct, const char *value);
void code_table_set_value (CodeTable *ct, gulong code, const char *new_value);
void code_table_chg_value (CodeTable *ct, const char *old_value, const char *new_value);
char *code_table_get_value (CodeTable *ct, gulong code);
gulong code_table_ins_entry (CodeTable *ct, const char *value);
gulong code_table_new_entry (CodeTable *ct, const char *value);
void code_table_del_by_code (CodeTable *ct, gulong code);
void code_table_del_by_value (CodeTable *ct, const char *value);

#endif /* CODE_TABLE_H */
