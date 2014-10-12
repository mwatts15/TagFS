#include <stdlib.h>
#include "log.h"
#include "sql.h"

int _sql_exec(sqlite3 *db, char *cmd, const char *file, int line_number)
{
    char *errmsg = NULL;
    int sqlite_res = sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
    if (sqlite_res != 0)
    {
        log_msg1(ERROR, file, line_number, "sqlite3_exec:%s", errmsg);
        sqlite3_free(errmsg);
    }
    return sqlite_res;
}
