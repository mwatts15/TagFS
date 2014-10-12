#include <stdlib.h>
#include "log.h"
#include "sql.h"

int sql_exec(sqlite3 *db, char *cmd)
{
    char *errmsg = NULL;
    int sqlite_res = sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
    if (sqlite_res != 0)
    {
        error(errmsg);
        sqlite3_free(errmsg);
    }
    return sqlite_res;
}
