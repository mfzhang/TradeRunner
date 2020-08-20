#include "strategy_redis_worker.h"
#include "my_strategy_manager.h"
#include "InstrumentJsonHelper.h"
#include "TradeRunnerArgus.h"
#include "ctp_message_def.h"
#include <boost/format.hpp>
#include "my_utils.h"
#include <boost/filesystem.hpp>
#include <json\json.h>
strategy_redis_worker::strategy_redis_worker(RedisClient &_redis): redis(_redis)
{
}


strategy_redis_worker::~strategy_redis_worker()
{
}

void strategy_redis_worker::get_ctp_instrument_value_simple(const char* instrumentid, const boost::function<void(const RedisValue &)> &handler)
{
	std::string strkey = boost::str(boost::format("%1%%2%") % instrumentid %ctp_message_def::KEY_CTP_INSTRUMENT_VALUE_SIMPLE_POSTFIX);
	RedisBuffer key(strkey.c_str());
	redis.command("GET", key, handler);
}

/*
多个实例一起运行，会导致数据错误，需要考虑同步问题
*/
void strategy_redis_worker::refresh_redis_content_ctp_strategy()
{
	std::string strValue = my_strategy_manager::get_mutable_instance().serializeStrategies();
	RedisBuffer rb(strValue);
	std::string strkey = boost::str(boost::format("%1%%2%") %ctp_message_def::LOCAL_STRATEGIES_SUFFIX  %my_utils::get_local_ip_address());
	RedisBuffer key(strkey.c_str());

	redis.command("SET", key, rb);

	//_redis_worker.publish_command()
}

void strategy_redis_worker::update_subscribe_instrument_ids(const char *investor)
{
	Json::Value root(Json::ValueType::arrayValue);

	std::set<std::string> ins = my_strategy_manager::get_mutable_instance().get_all_scripted_instrumntids();
	for each(std::string insid in ins)
	{
		Json::Value v(insid.c_str());
		root.append(v);
	}

	std::string content = root.toStyledString();
	RedisBuffer rb(content);
	std::string strkey = boost::str(boost::format("%1%_%2%") % ctp_message_def::SUBSCRIBE_MARKET_DATA  %investor);
	RedisBuffer key(strkey);
	redis.command("SET", key, rb);
}
void strategy_redis_worker::refresh_single_strategy_values(const char * filename, int buy, int sell)
{
	std::string strValue = boost::str(boost::format("{\"buy\":%1%,\"sell\":%2%}") % buy  %sell);
	RedisBuffer rb(strValue);
	boost::filesystem::path file(filename);
	
	std::vector<std::string> _inputs;
	boost::split(_inputs, file.stem().generic_string(), boost::is_any_of("_"), boost::token_compress_on);
	//inputs[0]表示id，redis的key搞不定中文的情况
	std::string keystr = _inputs[0] + "_strategy_info";
	RedisBuffer key(keystr.c_str());

	redis.command("SET", key, rb);
}
void strategy_redis_worker::get_local_ctp_strategies(const boost::function<void(const RedisValue &)> &handler)
{
	std::string strkey = boost::str(boost::format("%1%%2%") % ctp_message_def::LOCAL_STRATEGIES_SUFFIX  %my_utils::get_local_ip_address());
	RedisBuffer key(strkey.c_str());

	redis.command("GET", key, handler);
}