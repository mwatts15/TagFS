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

int _sql_next_row(sqlite3_stmt *stmt, const char *file, int line_number)
{
    int status = sqlite3_step(stmt);
    if ((status == SQLITE_ROW) || (status == SQLITE_DONE))
    {
        return status;
    }
    else
    {
        error("We didn't finish the select statemnt: %d",  status);
        return status;
    }
}
