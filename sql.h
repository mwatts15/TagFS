#ifndef SQL_H
#define SQL_H
#include <sqlite3.h>

int _sql_exec(sqlite3 *db, char *cmd, const char *file, int line_number);
#define sql_prepare(__db, __cmd, __stmt) \
    sqlite3_prepare_v2(__db, __cmd, -1, &(__stmt), NULL)
#define sql_exec(__db, __cmd) _sql_exec(__db, __cmd, __FILE__, __LINE__)

#endif /* SQL_H */

