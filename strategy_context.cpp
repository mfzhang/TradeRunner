#include "strategy_context.h"
#include <boost/thread/lock_guard.hpp>
#include "HistoryMysqlDb.h"

strategy_context::strategy_context()
{
	history_db = nullptr;
}


strategy_context::~strategy_context()
{
}


void strategy_context::update_instrument_simple_value(std::string &str,InstrumentSimpleValue &isv)
{
	boost::lock_guard<std::mutex> guard(my_mutex);
	m_allInstrumentSimpleValues[str] = isv;
}

bool strategy_context::get_instrument_simple_value(const char* ins, InstrumentSimpleValue &isv)
{
	boost::lock_guard<std::mutex> guard(my_mutex);
	if (m_allInstrumentSimpleValues.find(ins) == m_allInstrumentSimpleValues.end()) return false;
	InstrumentSimpleValue &inside = m_allInstrumentSimpleValues[ins];
	isv.open = inside.open;
	isv.close = inside.close;
	isv.high = inside.high;
	isv.low= inside.low;
	isv.tradingDay = inside.tradingDay;

	//É¾µô
	m_allInstrumentSimpleValues.erase(ins);
	return true;
}

HistoryMysqlDb& strategy_context::get_history_mysql_db()
{
	return *history_db;
}

bool strategy_context::connect_mysql(const std::string& connection_string, const std::string & user, const std::string &password)
{
	if (history_db == nullptr) {
		history_db = new HistoryMysqlDb();
	}
	
	if (!history_db->is_connected()) {
		history_db->Connect(connection_string, user, password);
		return history_db->is_connected();
	}

	return true;
}