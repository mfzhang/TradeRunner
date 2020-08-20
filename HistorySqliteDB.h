#ifndef HISTORY_SQLITE_DB_H__
#define HISTORY_SQLITE_DB_H__
#include "SqliteHelper.h"
#include <map>
#include <vector>
using namespace std;
class HistoryItem;

class HistorySqliteDB
{
private:
	map<string,map<int,vector<HistoryItem*>*>> m_allHistoryItems;

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

	static string getHistoryTable(int _nMinDual)
	{
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
		case 24*60:
			return TABLE_History1Day;
		case 24*60*7:
			return TABLE_History1Week;
		case 24*60*7*30:
			return TABLE_History1Mon;
		default:
			return "";
		}
	}

	vector<HistoryItem*>& getHistoryItems(string _Code,int _MinDual);
	static std::map<std::string, bool> instrument_item_history_k_line_receiving;
	// history db path
	static const string HISTORY_DB_PATH;
	static HistorySqliteDB& instance(string _ItemCode)
	{
		string str;
		for (size_t i = 0; i < _ItemCode.size(); ++i)
		{
			if (_ItemCode[i] > '9' || _ItemCode[i] < '0')
			{
				str.push_back(_ItemCode[i]);
			}
		}
		if (m_allHistorySqliteDBs.find(str) == m_allHistorySqliteDBs.end())
		{
			HistorySqliteDB *pDB = new HistorySqliteDB();
			string dbpath = HISTORY_DB_PATH +"History"+ str + ".sqlite";
			pDB->m_SqliteHelper.openDB(dbpath.c_str());
			m_allHistorySqliteDBs[str] = pDB;
		}
		return *m_allHistorySqliteDBs[str];
	}

	static HistorySqliteDB& instancefast(string _ItemCode)
	{
		
		if (m_allHistorySqliteDBs.find(_ItemCode) == m_allHistorySqliteDBs.end())
		{
			HistorySqliteDB *pDB = new HistorySqliteDB();
			string dbpath = HISTORY_DB_PATH + "History" + _ItemCode + ".sqlite";
			pDB->m_SqliteHelper.openDB(dbpath.c_str());
			m_allHistorySqliteDBs[_ItemCode] = pDB;
		}
		return *m_allHistorySqliteDBs[_ItemCode];
	}

	SQLiteHelper m_SqliteHelper;

	void SaveHistoryItem(HistoryItem &_Item);

	bool HistoryItemExist(HistoryItem &_item);

	void UpdateHistoryItem(HistoryItem &_item);

	void RuntimeHistoryItemReceived(HistoryItem &_item);
private:
	HistorySqliteDB();
	~HistorySqliteDB();
	// TO Delete
	static map<string, HistorySqliteDB*> m_allHistorySqliteDBs;
public:
	//不支持copy，所有的都在这里
	//vector<HistoryItem*>* getHistoryItemCopys(vector<HistoryItem*> *pSource);

	void CalculateMAIndexType(const char * ins, int cycle, int duration);
	void CalculateMACDIndexType(const char * ins, int cycle,int key1,int key2,int key3);

	
};
class InstrumentHistoryKLineReceivingMarker {
public:
	InstrumentHistoryKLineReceivingMarker(std::string _item) {
		instrument_id = _item;
		HistorySqliteDB::instrument_item_history_k_line_receiving[_item] = true;
	}
	~InstrumentHistoryKLineReceivingMarker() {
		HistorySqliteDB::instrument_item_history_k_line_receiving[instrument_id] = false;
	}
private:
	std::string instrument_id;
};

#endif