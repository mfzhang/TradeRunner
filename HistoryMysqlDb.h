#ifndef _HISTORY_MYSQL_DB__
#define HISTORY_MYSQL_DB__
#include <string>
#include "jdbc\mysql_connection.h"
#include "jdbc\mysql_driver.h"

#include <map>
#include <vector>
#include <string>
using namespace sql::mysql;
using namespace std;
class HistoryItem;
class HistoryMysqlDb
{
public:
	HistoryMysqlDb();
	virtual ~HistoryMysqlDb();

	void Connect(const std::string& connection_string, const std::string & user, const std::string &password);

	bool is_connected();
private:
	sql::Connection *mysql_connection;
	MySQL_Driver *mysql_driver;

private:
	map<string, map<int, vector<HistoryItem*>*>> m_allHistoryItems;

	// 临时储存最新的k线数据，来自redis，如果历史数据已经读入m_allHistoryItems，可以存完之后更新上面
	map<string, map<int, HistoryItem>> m_runtimeHistoryItems;
public:
	// history table name
	static const string TABLE_History1Min;
	static const string TABLE_History3Min;
	static const string TABLE_History5Min;
	static const string TABLE_History10Min;
	static const string TABLE_History15Min;
	static const string TABLE_History30Min;
	static const string TABLE_History1Hour;
	static const string TABLE_History3Hour;
	static const string TABLE_History1Day;
	static const string TABLE_History1Week;
	static const string TABLE_History1Mon;
	static const string SCHEMA_NAME;
	static string getHistoryTable(const string &_Code, int _nMinDual);
	/*{

		switch (_nMinDual)
		{
		case 1:
			return TABLE_History1Min;
		case 3:
			return TABLE_History3Min;
		case 5:
			return TABLE_History5Min;
		case 10:
			return TABLE_History10Min;
		case 15:
			return TABLE_History15Min;
		case 30:
			return TABLE_History30Min;
		case 60:
			return TABLE_History1Hour;
		case 180:
			return TABLE_History3Hour;
		case 24 * 60:
			return TABLE_History1Day;
		case 24 * 60 * 7:
			return TABLE_History1Week;
		case 24 * 60 * 7 * 30:
			return TABLE_History1Mon;
		default:
			return "";
		}
	}*/

	vector<HistoryItem*>& getHistoryItems(const string & _Code, int _MinDual);
	static std::map<std::string, bool> instrument_item_history_k_line_receiving;

	void SaveHistoryItem(const HistoryItem &_Item);

	bool HistoryItemExist(const HistoryItem &_item);

	void UpdateHistoryItem(const HistoryItem &_item);

	void RuntimeHistoryItemReceived(HistoryItem &_item);

	void CalculateMAIndexType(const char * ins, int cycle, int duration);
private:
	std::string _connection_string;
	std::string _user;
	std::string _password;
};
class InstrumentHistoryKLineReceivingMarker {
public:
	InstrumentHistoryKLineReceivingMarker(std::string _item) {
		instrument_id = _item;
		HistoryMysqlDb::instrument_item_history_k_line_receiving[_item] = true;
	}
	~InstrumentHistoryKLineReceivingMarker() {
		HistoryMysqlDb::instrument_item_history_k_line_receiving[instrument_id] = false;
	}
private:
	std::string instrument_id;
};
#endif