#include "code_table.h"
#include "util.h"

int main()
{
    CodeTable *ct = code_table_new();
    code_table_new_entry(ct, "file1");
    code_table_new_entry(ct, "file2");
    code_table_new_entry(ct, "file3");
    code_table_new_entry(ct, "file4");
    code_table_new_entry(ct, "file5");
    code_table_del_by_code(ct, 0);
    code_table_new_entry(ct, "file5");
    code_table_new_entry(ct, "file10");
    code_table_del_by_value(ct, "file5");
    code_table_new_entry(ct, "file12");
    code_table_new_entry(ct, "file11");

    print_hash(ct->reverse);
    return 0;
}
