#ifndef _SQL_H_
#define _SQL_H_
#include <glib.h>
#include <sqlite3.h>

int _sql_exec(sqlite3 *db, char *cmd, const char *file, int line_number);
int _sql_next_row(sqlite3_stmt *stmt, const char *file, int line_number);
int _sql_prepare (sqlite3 *db, const char *command, sqlite3_stmt **stmt, const char *file, int line_number);
int _sql_step (sqlite3_stmt *stmt, const char *file, int line_number);

#define sql_exec(__db, __cmd) _sql_exec(__db, __cmd, __FILE__, __LINE__)
#define sql_next_row(__stmt) _sql_next_row(__stmt, __FILE__, __LINE__)
#define sql_prepare(__db, __cmd, __stmt) _sql_prepare(__db, __cmd, &(__stmt), __FILE__, __LINE__)
#define sql_step(__stmt) _sql_step(__stmt,__FILE__, __LINE__)
void sql_begin_transaction(sqlite3 *db);
void sql_commit(sqlite3 *db);
sqlite3* sql_init (const char *db_fname);

/* Returns TRUE if the database was successfully initialized, and FALSE otherwise */
gboolean database_init(sqlite3 *db);
int database_backup (sqlite3 *db);
gboolean database_clear_backups (sqlite3 *db);

#define DB_VERSION 2
#define DB_VERSION_S "2"

#define BKP_PART ".bkp"

#endif /* _SQL_H_ */
