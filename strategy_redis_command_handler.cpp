#include "strategy_redis_command_handler.h"
#include "strategy_redis_worker.h"
#include <json\json.h>
#include "my_strategy_manager.h"
#define ELPP_THREAD_SAFE  
#include "easylogging++.h"
#include "strategy_context.h"
strategy_redis_command_handler::strategy_redis_command_handler(strategy_redis_worker &redis_worker):_redis_worker(redis_worker)
{
}
strategy_redis_command_handler::~strategy_redis_command_handler()
{
}

void strategy_redis_command_handler::ctp_instrument_simple_value_update_handler(const std::vector<char> &buf)
{
	std::string content(buf.begin(), buf.end());
	_redis_worker.get_ctp_instrument_value_simple(content.c_str(),boost::bind(&strategy_redis_command_handler::ctp_instrument_simple_value_result_handler,this,_1));
}

void strategy_redis_command_handler::ctp_instrument_simple_value_result_handler(const RedisValue &buf)
{
	std::string content(buf.toString());
	Json::Reader reader;
	Json::Value root;
	Json::Value d_s("");
	Json::Value d_d(0.0f);
	std::string iid = "";
	double _open, _high, _low, _close;
	if (reader.parse(content, root))
	{
		iid = root.get("id", d_s).asCString();
		if (iid.empty())return;

		/*std::map<std::string, InstrumentRuntime> & ins = TradeContext::get_mutable_instance().m_allInstrumentRuntimes;
		if (ins.find(iid) == ins.end()) return;*/
		Json::Value id = root.get("open", d_d);
		if (id == d_d) return;
		_open = id.asDouble();
		
		id = root.get("high", d_d);
		if (id == d_d) return;
		_high = id.asDouble();

		id = root.get("low", d_d);
		if (id == d_d) return;
		_low = id.asDouble();

		id = root.get("close", d_d);
		if (id == d_d) return;
		_close = id.asDouble();

		std::string _tradingDay = root.get("date", d_s).asCString();

		InstrumentSimpleValue insv;
		insv.open = _open;
		insv.close = _close;
		insv.high = _high;
		insv.low = _low;
		insv.tradingDay = _tradingDay;

		strategy_context::get_mutable_instance().update_instrument_simple_value(iid, insv);
		//my_strategy_manager::get_mutable_instance().execute(iid, _tradingDay, insv,_redis_worker);

		LOG(DEBUG) << iid << "(" << _tradingDay << "):" << _open << "," << _high << "," << _low << "," << _close;
		/*TradeContext::get_mutable_instance().m_allInstrumentRuntimes[iid].latestValue.open = _open;
		TradeContext::get_mutable_instance().m_allInstrumentRuntimes[iid].latestValue.close = _close;
		TradeContext::get_mutable_instance().m_allInstrumentRuntimes[iid].latestValue.high = _high;
		TradeContext::get_mutable_instance().m_allInstrumentRuntimes[iid].latestValue.low = _low;*/
	}
}
void strategy_redis_command_handler::event_strategy_updated_handler(const std::vector<char> &buf)
{
	std::string content(buf.begin(), buf.end());
	_redis_worker.get_local_ctp_strategies(boost::bind(&strategy_redis_command_handler::ctp_local_strategies_result_handler, this, _1));
}

void strategy_redis_command_handler::ctp_local_strategies_result_handler(const RedisValue &buf)
{
	my_future_strategy_type external_strategies;
	std::string content(buf.toString());
	Json::Reader reader;
	Json::Value root;
	if (reader.parse(content, root))
	{
		if (root.isArray())
		{
			for (Json::ArrayIndex  i = 0; i < root.size(); ++i)
			{
				// element的key是instrumentid
				std::map<std::string, std::vector<my_future_strategy>> element;
				if (root[i]["instruments"] != nullptr && root[(int)i]["instruments"].isArray())
				{
					// 这一层是instrumentid
					for (Json::ArrayIndex  j = 0; j < root[i]["instruments"].size(); ++j)
					{
						std::vector<my_future_strategy> strategies;
						if (root[i]["instruments"][j]["strategies"] != nullptr && root[i]["instruments"][j]["strategies"].isArray())
						{
							my_future_strategy strategy;
							for (int k = 0; k < root[i]["instruments"][j]["strategies"].size(); ++k)
							{
								strategy._instrumenId = root[i]["instruments"][j]["instrumentid"].asCString();
								strategy._investor = root[i] ["investor"].asCString();
								strategy._finished = root[i]["instruments"][j]["strategies"][k]["finished"].asBool();
								strategy._strategy_script = root[i]["instruments"][j]["strategies"][k]["script"].asCString();
							}
						}
						element["instruments"] = strategies;
					}
				
				}

				external_strategies[root[i]["investor"].asCString()] = element;
			}
		}
	}

	
	my_strategy_manager::get_mutable_instance().update_strategies_external(external_strategies);
}