#ifndef SQLITE_HELPER_H__
#define SQLITE_HELPER_H__
#include "lib\sqlite\sqlite3.h"

class SQLiteHelper
{
public:
	SQLiteHelper();
	virtual ~SQLiteHelper();
	sqlite3 *db;
	void execSQL(const char *sql);
	char**rawQuery(const char *sql, int *row, int *column, char **result);
	void openDB(const char *path);
	void closeDB();
};

#endif
