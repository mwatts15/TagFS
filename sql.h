#ifndef SQL_H
#define SQL_H
#include <sqlite3.h>

int sql_exec(sqlite3 *db, char *cmd);
#define sql_prepare(__db, __cmd, __stmt) \
    sqlite3_prepare_v2(__db, __cmd, -1, &(__stmt), NULL)

#endif /* SQL_H */

