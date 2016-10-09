#ifndef PRIVATE_SQL_H
#define PRIVATE_SQL_H

int _get_version(sqlite3 *db);
int _set_version(sqlite3 *db);
int _set_version0(sqlite3 *db, int version);
int try_upgrade_db0 (sqlite3 *db, int target_version);
#endif /* PRIVATE_SQL_H */

