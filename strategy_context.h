#ifndef STRATEGYCONTEXT_H
#define STRATEGYCONTEXT_H
#include <boost/serialization/singleton.hpp> 
#include "InstrumentRuntime.h"
#include <mutex>
class HistoryMysqlDb;
class strategy_context :public boost::serialization::singleton<strategy_context>
{
public:
	strategy_context();
	~strategy_context();



	void update_instrument_simple_value(std::string &str, InstrumentSimpleValue &isv);

	bool get_instrument_simple_value(const char* ins, InstrumentSimpleValue &isv);

	HistoryMysqlDb& get_history_mysql_db();

	bool connect_mysql(const std::string& connection_string, const std::string & user, const std::string &password);
private:
	std::map<std::string, InstrumentSimpleValue> m_allInstrumentSimpleValues;

	std::mutex my_mutex;

	HistoryMysqlDb * history_db;
};
#endif 