#include "stdafx.h"
#include "HistorySqliteDB.h"
#include "HistoryItem.h"
#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost\format.hpp>
#define ELPP_THREAD_SAFE  
#include "easylogging++.h"
#include "my_utils.h"
using namespace boost::posix_time;
//using namespace boost::gregorian;

using namespace boost;
map<string, HistorySqliteDB*> HistorySqliteDB::m_allHistorySqliteDBs;
std::map<std::string, bool> HistorySqliteDB::instrument_item_history_k_line_receiving;
//const string HistorySqliteDB::HISTORY_DB_PATH = "C:\\wodequant\\database\\";
//const string HistorySqliteDB::HISTORY_DB_PATH = std::string(my_utils::K_WODEQUANT_FOLDER) +"database\\";
const string HistorySqliteDB::HISTORY_DB_PATH = "C:\\wqd\\database\\";
const string HistorySqliteDB::TABLE_History1Min="History_1Min";
const string HistorySqliteDB::TABLE_History3Min = "History_3Min";
const string HistorySqliteDB::TABLE_History5Min = "History_5Min";
const string HistorySqliteDB::TABLE_History10Min = "History_10Min";
const string HistorySqliteDB::TABLE_History15Min = "History_15Min";
const string HistorySqliteDB::TABLE_History30Min = "History_30Min";
const string HistorySqliteDB::TABLE_History1Hour = "History_1Hour";
const string HistorySqliteDB::TABLE_History3Hour = "History_3Hour";
const string HistorySqliteDB::TABLE_History1Day = "History_1Day";
const string HistorySqliteDB::TABLE_History1Week = "History_1Week";
const string HistorySqliteDB::TABLE_History1Mon = "History_1Mon";

using namespace boost::posix_time;

HistorySqliteDB::HistorySqliteDB()
{
}


HistorySqliteDB::~HistorySqliteDB()
{
}


/*vector<HistoryItem*>* HistorySqliteDB::getHistoryItemCopys(vector<HistoryItem*> *pSource)
{
	if (pSource == NULL) return NULL;

	vector<HistoryItem*>* pRet = new vector<HistoryItem*>();

	for (size_t i = 0; i < pSource->size(); ++i)
	{
		HistoryItem *pItem = new HistoryItem(*(*pSource)[i]);
		pRet->push_back(pItem);
	}

	return pRet;
}*/
vector<HistoryItem*>& HistorySqliteDB::getHistoryItems(string _Code, int _MinDual)
{
	if (m_allHistoryItems.find(_Code) != m_allHistoryItems.end())
	{
		if (m_allHistoryItems[_Code].find(_MinDual) != m_allHistoryItems[_Code].end())
		{
			return *m_allHistoryItems[_Code][_MinDual];
		}
	}

	vector<HistoryItem*>*historyItems = new vector<HistoryItem*>();
	m_allHistoryItems[_Code][_MinDual] = historyItems;


	string str("select InstrumentCode, DateTime, Open, High, Low, Close, Volume from ");
	str += getHistoryTable(_MinDual);
	str += " where InstrumentCode = '";
	str += _Code;
	str += "' ";
	str += " order by DateTime";
	int column, row;
	char *pResult = "i";
	char **result = m_SqliteHelper.rawQuery(str.c_str(), &row, &column, &pResult);

	for (int j = (row-5000>0?(row-5000):1); j <= row; ++j)//只取前5000条数据
	{
		HistoryItem *history = new HistoryItem();
		history->m_nMinuteDual = _MinDual;
		history->m_InstrumentCode = _Code;
		history->m_Item = _Code;
		if (result[j*column + 1] != NULL)
		{
			string timestr = result[j*column + 1];
			if (timestr.size() == 10)
				timestr += " 00:00:00.000";
			history->m_StartTime = time_from_string(timestr);
			history->m_EndTime = history->m_StartTime + minutes(_MinDual) - seconds(1);
		}

		if (result[j*column + 2] != NULL)
		{
			history->m_OpenPrice = atof(result[j*column + 2]);
		}

		if (result[j*column + 3] != NULL)
		{
			history->m_HighPrice = atof(result[j*column + 3]);
		}

		if (result[j*column + 4] != NULL)
		{
			history->m_LowPrice= atof(result[j*column + 4]);
		}
		if (result[j*column + 5] != NULL)
		{
			history->m_ClosePrice= atof(result[j*column + 5]);
		}
		if (result[j*column + 6] != NULL)
		{
			history->m_nVolume = atoi(result[j*column + 6]);
		}
		if (history->m_OpenPrice > 100
			&& history->m_ClosePrice > 100
			&& history->m_HighPrice > 100
			&& history->m_LowPrice > 100
			&& history->m_nVolume > 0)
		{
			m_allHistoryItems[_Code][_MinDual]->push_back(history);
		}
		else delete history;

	}
	if(m_runtimeHistoryItems.find(_Code)!= m_runtimeHistoryItems.end()
		&& m_runtimeHistoryItems[_Code].find(_MinDual) != m_runtimeHistoryItems[_Code].end())
	// 合并到history列表
	RuntimeHistoryItemReceived(m_runtimeHistoryItems[_Code][_MinDual]);

	return *m_allHistoryItems[_Code][_MinDual];
}

void HistorySqliteDB::SaveHistoryItem(HistoryItem &_Item)
{
	try{

		string sql = str(boost::format("insert into %1%(InstrumentCode, DateTime, Open, High, Low, Close, Volume) values('%2%','%3%', %4%, %5%, %6%, %7%, %8%)")
			% getHistoryTable(_Item.m_nMinuteDual)
			% _Item.m_Item
			%str(boost::format("%1% %2$02d:%3$02d:%4$02d") % to_iso_extended_string(_Item.m_StartTime.date()) % _Item.m_StartTime.time_of_day().hours() % _Item.m_StartTime.time_of_day().minutes() % _Item.m_StartTime.time_of_day().seconds())
			% _Item.m_OpenPrice
			%_Item.m_HighPrice
			%_Item.m_LowPrice
			%_Item.m_ClosePrice
			%_Item.m_nVolume);


		m_SqliteHelper.execSQL(sql.c_str());
	}
	catch (...){}
}


bool HistorySqliteDB::HistoryItemExist(HistoryItem &_Item)
{
	try {
		string sql = str(boost::format("select * from %1% where InstrumentCode='%2%' and datetime([DateTime])='%3'")
			% getHistoryTable(_Item.m_nMinuteDual)
			% _Item.m_Item
			%str(boost::format("%1% %2$02d:%3$02d:%4$02d") % to_iso_extended_string(_Item.m_StartTime.date()) % _Item.m_StartTime.time_of_day().hours() % _Item.m_StartTime.time_of_day().minutes() % _Item.m_StartTime.time_of_day().seconds()));
		char *pResult = "i";
		int column, row;
		char **result = m_SqliteHelper.rawQuery(sql.c_str(), &row, &column, &pResult);

		if (row > 0) {
			return true;
		}
	}
	catch (...) {}
	return false;
}

void HistorySqliteDB::UpdateHistoryItem(HistoryItem &_Item)
{
	try {

		string sql = str(boost::format("update %1% set Open=%4%, High=%5%, Low=%6%, Close=%7%, Volume=%8% where where InstrumentCode='%2%' and datetime([DateTime])='%3'")
			% getHistoryTable(_Item.m_nMinuteDual)
			% _Item.m_Item
			%str(boost::format("%1% %2$02d:%3$02d:%4$02d") % to_iso_extended_string(_Item.m_StartTime.date()) % _Item.m_StartTime.time_of_day().hours() % _Item.m_StartTime.time_of_day().minutes() % _Item.m_StartTime.time_of_day().seconds())
			% _Item.m_OpenPrice
			%_Item.m_HighPrice
			%_Item.m_LowPrice
			%_Item.m_ClosePrice
			%_Item.m_nVolume);

		m_SqliteHelper.execSQL(sql.c_str());
	}
	catch (...) {}
}
void HistorySqliteDB::CalculateMAIndexType(const char * ins, int cycle, int duration)
{
	vector<HistoryItem*>&histories = getHistoryItems(ins, cycle);

	if (histories.empty()) return;
	
	//已经有了
	if (histories[0]->ma_values.find(duration) != histories[0]->ma_values.end()) return;

	for (size_t i = 0; i < histories.size(); ++i)
	{
		if (i == 0)
		{
			histories[i]->ma_values[duration] = histories[i]->m_ClosePrice;
			continue;
		}
		//暂时只支持螺纹，20天均线
		if (i < duration)
		{
			histories[i]->ma_values[duration] = ((double)i * histories[i-1]->ma_values[duration] + histories[i]->m_ClosePrice) / (i + 1);
		}
		else
		{
			histories[i]->ma_values[duration] = ((double)duration * histories[i-1]->ma_values[duration] - histories[i - duration]->m_ClosePrice + histories[i]->m_ClosePrice) / ((double)duration);
		}
	}
}

void HistorySqliteDB::CalculateMACDIndexType(const char * ins, int cycle, int key1, int key2, int key3)
{

}

void HistorySqliteDB::RuntimeHistoryItemReceived(HistoryItem &_item)
{
	InstrumentHistoryKLineReceivingMarker marker(_item.m_Item);

	m_runtimeHistoryItems[_item.m_Item][_item.m_nMinuteDual] = _item;

	//查找合并
	if (m_allHistoryItems.find(_item.m_Item) != m_allHistoryItems.end())
	{
		if (m_allHistoryItems[_item.m_Item].find(_item.m_nMinuteDual) != m_allHistoryItems[_item.m_Item].end())
		{
			vector<HistoryItem*>&histories = *m_allHistoryItems[_item.m_Item][_item.m_nMinuteDual];
			if (histories.back()->m_StartTime > _item.m_StartTime)
				return;

			if (histories.back()->m_StartTime == _item.m_StartTime)
			{
				*histories.back() = _item;
				// 重新计算ma
				std::map<int, float>::iterator it;
				for (it = histories[0]->ma_values.begin();
					it != histories[0]->ma_values.end();
					++it) {
					//计算最后一个的ma_values
					size_t i = histories.size() - 1;

					if (i < it->first)
					{
						histories[i]->ma_values[it->first] = ((double)i * histories[i - 1]->ma_values[it->first] + histories[i]->m_ClosePrice) / (i + 1);
					}
					else
					{
						histories[i]->ma_values[it->first] = ((double)it->first * histories[i - 1]->ma_values[it->first] - histories[i - it->first]->m_ClosePrice + histories[i]->m_ClosePrice) / ((double)it->first);
					}

					LOG(INFO) << "ma value----" << histories[i]->m_StartTime << "," << histories[i]->m_Item << "," << histories[i]->m_nMinuteDual << ":::" << it->first << "," << histories[i]->ma_values[it->first];
				}
			}
			else if (histories.back()->m_StartTime < _item.m_StartTime)
			{
				//先计算ma
				std::map<int, float>::iterator it;
				for (it = histories[0]->ma_values.begin();
					it != histories[0]->ma_values.end();
					++it) {
					//计算最后一个的ma_values
					size_t i = histories.size();

					if (i < it->first)
					{
						_item.ma_values[it->first] = ((double)i * histories[i - 1]->ma_values[it->first] + _item.m_ClosePrice) / (i + 1);
					}
					else
					{
						_item.ma_values[it->first] = ((double)it->first * histories[i - 1]->ma_values[it->first] - histories[i - it->first]->m_ClosePrice + _item.m_ClosePrice) / ((double)it->first);
					}

					LOG(INFO) << "ma value----" << _item.m_StartTime << "," << _item.m_Item << "," << _item.m_nMinuteDual << ":::" << it->first << "," << _item.ma_values[it->first];
				}

				HistoryItem *p = new HistoryItem(_item);
				histories.push_back(p);
			}

			
			

		}
	}
}