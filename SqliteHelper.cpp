#include "stdafx.h"
#include "SqliteHelper.h"

#include <iostream>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SQLiteHelper::SQLiteHelper()
{

}

SQLiteHelper::~SQLiteHelper()
{

}
void SQLiteHelper::execSQL(const char *sql)
{
	sqlite3_exec(db, sql, 0, 0, 0);
}
char **SQLiteHelper::rawQuery(const char *sql, int *row, int *column, char **result)
{
	sqlite3_get_table(db, sql, &result, row, column, 0);
	return result;
}
void SQLiteHelper::openDB(const char *path)
{
	int last = sqlite3_open(path, &db);
	if (SQLITE_OK != last)
	{
		std::cout << "打开数据库出错" << endl;
		return;
	}
}
void SQLiteHelper::closeDB()
{
	sqlite3_close(db);
}
