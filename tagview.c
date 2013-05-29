#include "tagview.h"
//#include "query.h"
#include "tagdb_util.h"

int main ()
{
/* menu
 * Offer options like list files, change query, etc.
 */
    TagDB *db = tagdb_load("tagfs.db");
    //result_t *r = tagdb_query(db, "FILE SEARCH @all");
    //res_info(r,(printer) printf);
    GList *l = tagdb_all_files(db);
    print_list(l,file_to_string);
    return 0;
}
