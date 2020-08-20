#include "HistoryMysqlDb.h"
#include "HistoryItem.h"
#include "my_utils.h"
#include "jdbc\cppconn\statement.h"
#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost\format.hpp>
#define ELPP_THREAD_SAFE  
#include "easylogging++.h"


using namespace boost::posix_time;
using namespace boost;

std::map<std::string, bool> HistoryMysqlDb::instrument_item_history_k_line_receiving;

const string HistoryMysqlDb::TABLE_History1Min = "History_1Min";
const string HistoryMysqlDb::TABLE_History3Min = "History_3Min";
const string HistoryMysqlDb::TABLE_History5Min = "History_5Min";
const string HistoryMysqlDb::TABLE_History10Min = "History_10Min";
const string HistoryMysqlDb::TABLE_History15Min = "History_15Min";
const string HistoryMysqlDb::TABLE_History30Min = "History_30Min";
const string HistoryMysqlDb::TABLE_History1Hour = "History_1Hour";
const string HistoryMysqlDb::TABLE_History3Hour = "History_3Hour";
const string HistoryMysqlDb::TABLE_History1Day = "History_1Day";
const string HistoryMysqlDb::TABLE_History1Week = "History_1Week";
const string HistoryMysqlDb::TABLE_History1Mon = "History_1Mon";
const string HistoryMysqlDb::SCHEMA_NAME = "quant";

HistoryMysqlDb::HistoryMysqlDb()
{
	mysql_connection = nullptr;
	mysql_driver = nullptr;
}


HistoryMysqlDb::~HistoryMysqlDb()
{
    if (is_connected())
    {
        //mysql_connection->close();
    }
}


void HistoryMysqlDb::Connect(const std::string& connection_string, const std::string & user, const std::string &password)
{
	if (is_connected())
		return;
	try {
		_connection_string = connection_string;
		_user = user;
		_password = password;

		mysql_driver = sql::mysql::get_mysql_driver_instance();
		mysql_connection = mysql_driver->connect(connection_string, user, password);

	}
	catch (std::exception e) {
		std::cout << e.what();
	}
	catch(...){
	
	}
}

bool HistoryMysqlDb::is_connected() {
	if (mysql_connection != nullptr
		&&mysql_connection->isValid()) return true;

	return false;
}

vector<HistoryItem*>& HistoryMysqlDb::getHistoryItems(const string& _Code, int _MinDual)
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


	if (!is_connected()) {
		Connect(_connection_string, _user, _password);
		if(!is_connected())
			throw new std::exception("数据库没有连接");
	}

	std::string table = getHistoryTable (_Code, _MinDual);

	sql::Statement *stmt = nullptr;
	sql::ResultSet  *res;

	stmt = mysql_connection->createStatement();
	stmt->execute("use "+ SCHEMA_NAME);
	
	string str("select InstrumentCode, DateTime, Open, High, Low, Close, Volume from ");
	str += table;
	str += " where InstrumentCode = '";
	str += _Code;
	str += "' ";
	str += " order by DateTime desc";

	res = stmt->executeQuery(str);
	while (res->next()) {
		HistoryItem *history = new HistoryItem();
		history->m_nMinuteDual = _MinDual;
		history->m_InstrumentCode = _Code;
		history->m_Item = _Code;

		std::string timestr = res->getString("DateTime");

		if (timestr.size() == 10)
			timestr += " 00:00:00.000";
		history->m_StartTime = time_from_string(timestr);

		history->m_EndTime = history->m_StartTime + minutes(_MinDual) - seconds(1);

		history->m_OpenPrice = res->getDouble("Open");
		history->m_HighPrice = res->getDouble("High");
		history->m_LowPrice = res->getDouble("Low");
		history->m_ClosePrice = res->getDouble("Close");
		history->m_nVolume = res->getDouble("Volume");

		if (history->m_OpenPrice > 100
			&& history->m_ClosePrice > 100
			&& history->m_HighPrice > 100
			&& history->m_LowPrice > 100
			&& history->m_nVolume > 0)
		{
			//m_allHistoryItems[_Code][_MinDual]->push_back(history);
            m_allHistoryItems[_Code][_MinDual]->insert(m_allHistoryItems[_Code][_MinDual]->begin(), history);

			//只取前5000条
			if (m_allHistoryItems[_Code][_MinDual]->size() >= 7000) break;
		}
		else delete history;
	}


	delete res;
	delete stmt;

    if (m_runtimeHistoryItems.find(_Code) != m_runtimeHistoryItems.end()
        && m_runtimeHistoryItems[_Code].find(_MinDual) != m_runtimeHistoryItems[_Code].end())
        // 合并到history列表
        RuntimeHistoryItemReceived(m_runtimeHistoryItems[_Code][_MinDual]);

    return *m_allHistoryItems[_Code][_MinDual];
}
void HistoryMysqlDb::SaveHistoryItem(const HistoryItem &_Item)
{
	if (!is_connected()) {
		Connect(_connection_string, _user, _password);
		if (!is_connected())
			throw new std::exception("数据库没有连接");
	}
	try {

		string sql = str(boost::format("insert into %1%(InstrumentCode, DateTime, Open, High, Low, Close, Volume) values('%2%','%3%', %4%, %5%, %6%, %7%, %8%)")
			% getHistoryTable(_Item.m_InstrumentCode,_Item.m_nMinuteDual)
			% _Item.m_Item
			%str(boost::format("%1% %2$02d:%3$02d:%4$02d") % to_iso_extended_string(_Item.m_StartTime.date()) % _Item.m_StartTime.time_of_day().hours() % _Item.m_StartTime.time_of_day().minutes() % _Item.m_StartTime.time_of_day().seconds())
			% _Item.m_OpenPrice
			%_Item.m_HighPrice
			%_Item.m_LowPrice
			%_Item.m_ClosePrice
			%_Item.m_nVolume);

		LOG(ERROR) << sql;
		//m_SqliteHelper.execSQL(sql.c_str());
		sql::Statement *stmt = nullptr; 
		stmt = mysql_connection->createStatement();
		stmt->execute("use " + SCHEMA_NAME);
		stmt->execute(sql);

		delete stmt;
	}
	catch (std::exception e) {
		std::cout << e.what();
	}
	catch (...) {}
}

bool HistoryMysqlDb::HistoryItemExist(const HistoryItem &_Item)
{
	if (!is_connected()) {
		Connect(_connection_string, _user, _password);
		if (!is_connected())
			throw new std::exception("数据库没有连接");
	}
	try {
		string sql = str(boost::format("select * from %1% where InstrumentCode='%2%' and DateTime='%3%'")
			% getHistoryTable(_Item.m_InstrumentCode, _Item.m_nMinuteDual)
			% _Item.m_Item
			%str(boost::format("%1% %2$02d:%3$02d:%4$02d") % to_iso_extended_string(_Item.m_StartTime.date()) % _Item.m_StartTime.time_of_day().hours() % _Item.m_StartTime.time_of_day().minutes() % _Item.m_StartTime.time_of_day().seconds()));
		
		LOG(ERROR) << sql;
		sql::Statement *stmt = nullptr;
		sql::ResultSet  *res;

		stmt = mysql_connection->createStatement();
		stmt->execute("use " + SCHEMA_NAME);

		res = stmt->executeQuery(sql);
		if (res->next())
		{
			delete res;
			delete stmt;
			return true;
		}
	}
	catch (std::exception e) {
		std::cout << e.what();
	}
	catch (...) {}
	return false;
}

void HistoryMysqlDb::UpdateHistoryItem(const HistoryItem &_Item)
{
	if (!is_connected()) {
		Connect(_connection_string, _user, _password);
		if (!is_connected())
			throw new std::exception("数据库没有连接");
	}
	try {

		string sql = str(boost::format("update %1% set Open=%4%, High=%5%, Low=%6%, Close=%7%, Volume=%8% where InstrumentCode='%2%' and DateTime='%3%'")
			% getHistoryTable(_Item.m_InstrumentCode, _Item.m_nMinuteDual)
			% _Item.m_Item
			%str(boost::format("%1% %2$02d:%3$02d:%4$02d") % to_iso_extended_string(_Item.m_StartTime.date()) % _Item.m_StartTime.time_of_day().hours() % _Item.m_StartTime.time_of_day().minutes() % _Item.m_StartTime.time_of_day().seconds())
			% _Item.m_OpenPrice
			%_Item.m_HighPrice
			%_Item.m_LowPrice
			%_Item.m_ClosePrice
			%_Item.m_nVolume);
		LOG(ERROR) << sql;
		sql::Statement *stmt = nullptr;
		stmt = mysql_connection->createStatement();
		stmt->execute("use " + SCHEMA_NAME);
		stmt->executeUpdate(sql);
		delete stmt;
	}
	catch (std::exception e) {
		std::cout << e.what();
	}
	catch (...) {}
}

void HistoryMysqlDb::RuntimeHistoryItemReceived(HistoryItem &_item)
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

string HistoryMysqlDb::getHistoryTable(const string & _Code, int _nMinDual)
{

	const item_code_class &ic = my_utils::get_item_code(_Code.c_str());
    
	switch (_nMinDual)
	{
	case 1:
		return ic.name+ "_"+ TABLE_History1Min;
	case 3:
		return ic.name + "_" + TABLE_History3Min;
	case 5:
		return ic.name + "_" + TABLE_History5Min;
	case 10:
		return ic.name + "_" + TABLE_History10Min;
	case 15:
		return ic.name + "_" + TABLE_History15Min;
	case 30:
		return ic.name + "_" + TABLE_History30Min;
	case 60:
		return ic.name + "_" + TABLE_History1Hour;
	case 180:
		return ic.name + "_" + TABLE_History3Hour;
	case 24 * 60:
		return ic.name + "_" + TABLE_History1Day;
	case 24 * 60 * 7:
		return ic.name + "_" + TABLE_History1Week;
	case 24 * 60 * 7 * 30:
		return ic.name + "_" + TABLE_History1Mon;
	default:
		return "";
	}
}

void HistoryMysqlDb::CalculateMAIndexType(const char * ins, int cycle, int duration)
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
			histories[i]->ma_values[duration] = ((double)i * histories[i - 1]->ma_values[duration] + histories[i]->m_ClosePrice) / (i + 1);
		}
		else
		{
			histories[i]->ma_values[duration] = ((double)duration * histories[i - 1]->ma_values[duration] - histories[i - duration]->m_ClosePrice + histories[i]->m_ClosePrice) / ((double)duration);
		}
	}
}