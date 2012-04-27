#include "code_table.h"

CodeTable *code_table_new ()
{
    CodeTable *res = (CodeTable*) malloc(sizeof(CodeTable));
    res->forward = g_hash_table_new(g_direct_hash, g_direct_equal);
    res->reverse = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    res->size = 0;
    res->last_id = 0; // the keeps the last id assigned
    return res;
}

int code_table_get_code (CodeTable *ct, const char *value)
{
    return GPOINTER_TO_INT(g_hash_table_lookup(ct->reverse, value));
}

void code_table_chg_value (CodeTable *ct, const char *old, const char *new)
{
    int code = code_table_get_code (ct, old);
    code_table_set_value(ct, code, new);
}

void code_table_set_value (CodeTable *ct, int code, const char *new_value)
{
    char *vcopy = g_strdup(new_value);
    char *old_value = g_hash_table_lookup(ct->forward, GINT_TO_POINTER(code));
    g_hash_table_insert(ct->reverse, vcopy, GINT_TO_POINTER(code));
    g_hash_table_remove(ct->reverse, old_value);
    g_hash_table_insert(ct->forward, GINT_TO_POINTER(code), vcopy);
}

char *code_table_get_value (CodeTable *ct, int code)
{
    // fail if code == 0
    // => null pointer
    if (code == 0)
    {
        return NULL;
    }
    return g_hash_table_lookup(ct->forward, GINT_TO_POINTER(code));
}

// creates or changes an entry
// returns the new id
int code_table_ins_entry (CodeTable *ct, const char *value)
{
    // you can't insert the same string twice
    if (g_hash_table_lookup(ct->reverse, value) != NULL)
    {
        return code_table_get_code(ct, value);
    }
    ct->last_id++;
    ct->size++;
    int code = ct->last_id;
    char *vcopy = g_strdup(value);
    g_hash_table_insert(ct->reverse, vcopy, GINT_TO_POINTER(code));
    g_hash_table_insert(ct->forward, GINT_TO_POINTER(code), vcopy);
    return code;
}

// makes a new entry from ct->last_id
int code_table_new_entry (CodeTable *ct, const char *value)
{
    return code_table_ins_entry(ct, value);
}

void _code_table_delete (CodeTable *ct, int code, const char *value)
{
    gboolean a = g_hash_table_remove(ct->forward, GINT_TO_POINTER(code));
    gboolean b = g_hash_table_remove(ct->reverse, value); // frees value
    if ((!a && !b) || (b && a)) // both succeeded or failed
    {
        ct->size--;
    }
    else
    {
        perror("CODE_TABLE_DELETE has failed to remove");
    }
}

void code_table_del_by_code (CodeTable *ct, int code)
{
    // fail if code == 0
    // => null pointer
    if (code == 0)
    {
        return;
    }
    char *value = g_hash_table_lookup(ct->forward, GINT_TO_POINTER(code));
    _code_table_delete(ct, code, value);
}

void code_table_del_by_value (CodeTable *ct, const char *value)
{
    int code = GPOINTER_TO_INT(g_hash_table_lookup(ct->reverse, value));
    _code_table_delete(ct, code, value);
}
