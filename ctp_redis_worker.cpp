#include "ctp_redis_worker.h"
#include "InstrumentJsonHelper.h"
#include "TradeRunnerArgus.h"
#include "ctp_message_def.h"
#include <boost/format.hpp>
#include "TradeContext.h"
#include "my_utils.h"
ctp_redis_worker::ctp_redis_worker(RedisClient &_redis): redis(_redis)
{
}


ctp_redis_worker::~ctp_redis_worker()
{
}

void ctp_redis_worker::update_ctp_instrument_ids()
{
	Json::Value root(Json::ValueType::arrayValue);
	std::map<std::string, InstrumentRuntime>::iterator it;
	for (it = TradeContext::get_mutable_instance().m_allInstrumentRuntimes.begin();
		it != TradeContext::get_mutable_instance().m_allInstrumentRuntimes.end();
		++it)
	{
		Json::Value val(it->first);
		root.append(val);
	}

	std::string content = root.toStyledString();
	RedisBuffer rb(content);
	RedisBuffer key(ctp_message_def::KEY_CTP_INSTRUMENT_IDS);
	redis.command("SET",key , rb);
	redis.publish(ctp_message_def::KEY_CTP_INSTRUMENT_IDS, key);
}

void ctp_redis_worker::get_ctp_instrument_ids(const boost::function<void(const RedisValue &)> &handler)
{
	RedisBuffer key(ctp_message_def::KEY_CTP_INSTRUMENT_IDS);
	redis.command("GET", key, handler);

}

void ctp_redis_worker::update_ctp_account_login_info(const char * investor, const char*str)
{
	Json::Value root;
	root["time"] = my_utils::get_local_time();
	root["message"] = str;
	std::string strkey = boost::str(boost::format("%1%%2%") % investor %ctp_message_def::KEY_ACCOUNT_LOGIN_INFO_SUFFIX);
	RedisBuffer key(strkey);
	std::string strValue = root.toStyledString();
	RedisBuffer rb(strValue);
	redis.command("SET", key, rb);
}

// update account info to redis
void ctp_redis_worker::update_ctp_account_info(const CThostFtdcTradingAccountField &pField)
{

	Json::Value root,acc;
	instrument_json_helper::SerializeJsonCThostFtdcTradingAccountField(acc, pField);
	root["time"] = my_utils::get_local_time();
	root["account_info"] = acc;

	std::string strkey = boost::str(boost::format("%1%%2%") % pField.AccountID %ctp_message_def::KEY_ACCOUNT_INFO_SUFFIX);
	RedisBuffer key(strkey);
	std::string strValue = root.toStyledString();
	RedisBuffer rb(strValue);
	redis.command("SET", key, rb);
}

// update 持仓
void ctp_redis_worker::update_ctp_CThostFtdcInvestorPositionField(const char * investor, const char* instrumentid, const TwoDirectionsCThostFtdcInvestorPositionFieldMap &fields)
{
	std::string strkey = boost::str(boost::format("%1%%2%%3%") % investor %ctp_message_def::KEY_ACCOUNT_INVESTORPOSITIONFIELD_SUFFIX %instrumentid);
	RedisBuffer key(strkey.c_str());

	if (fields.empty())
	{
		redis.command("del", key);
		return;
	}

	Json::Value root,acc(Json::ValueType::arrayValue);
	root["time"] = my_utils::get_local_time();
	instrument_json_helper::SerializeJsonCThostFtdcInvestorPositionField(acc, fields);
	root["fields"] = acc;
	
	std::string strValue = root.toStyledString();
	RedisBuffer rb(strValue);

	redis.command("SET", key, rb);
}

// update 报单
void ctp_redis_worker::update_ctp_CThostFtdcOrderField(const char* suffix, const char* investor, const char* instrumentid, const std::vector<MyCThostFtdcOrderField> &fields)
{
	//std::string strkey = boost::str(boost::format("%1%%2%%3%") % investor %ctp_message_def::KEY_ACCOUNT_CTHOSTFTDCORDERFIELD_SUFFIX %instrumentid);
	std::string strkey = boost::str(boost::format("%1%%2%%3%") % investor %suffix %instrumentid);
	RedisBuffer key(strkey.c_str());
	if (fields.empty())
	{
		redis.command("del", key);
		return;
	}

	Json::Value root, acc(Json::ValueType::arrayValue);

	root["time"] = my_utils::get_local_time();
	
	instrument_json_helper::SerializeJsonCThostFtdcOrderField(acc, fields);
	root["fields"] = acc;

	std::string strValue = root.toStyledString();
	RedisBuffer rb(strValue);
	redis.command("SET", key, rb);
}

// update 成交记录
void ctp_redis_worker::update_ctp_CThostFtdcTradeField(const char* investor, const char* instrumentid, const std::vector<CThostFtdcTradeField > &fields)
{
	std::string strkey = boost::str(boost::format("%1%%2%%3%") % investor %ctp_message_def::KEY_ACCOUNT_CTHOSTFTDCTRADEFIELD_SUFFIX %instrumentid);
	RedisBuffer key(strkey.c_str());
	if (fields.empty())
	{
		redis.command("del", key);
		return;
	}

	Json::Value root, acc(Json::ValueType::arrayValue);

	root["time"] = my_utils::get_local_time();
	instrument_json_helper::SerializeJsonCThostFtdcTradeField(acc, fields);
	root["fields"] = acc;

	std::string strValue = root.toStyledString();
	RedisBuffer rb(strValue);
	redis.command("SET", key, rb);
}

// update 报单错误
void ctp_redis_worker::update_ctp_CThostFtdcInputOrderField(const char* investor, const char* instrumentid, const std::vector<CThostFtdcInputOrderField> &fields)
{
	std::string strkey = boost::str(boost::format("%1%%2%%3%") % investor %ctp_message_def::KEY_ACCOUNT_CTHOSTFTDCINPUTORDERFIELD_SUFFIX %instrumentid);
	RedisBuffer key(strkey.c_str());
	if (fields.empty())
	{
		redis.command("del", key);
		return;
	}

	Json::Value root, acc(Json::ValueType::arrayValue);

	root["time"] = my_utils::get_local_time();
	instrument_json_helper::SerializeJsonCThostFtdcInputOrderField(acc, fields);
	root["fields"] = acc;

	std::string strValue = root.toStyledString();
	RedisBuffer rb(strValue);
	redis.command("SET", key, rb);
}


// 持仓
void ctp_redis_worker::update_ctp_CThostFtdcInvestorPositionField(const char * investor)
{
	std::map<std::string, InstrumentRuntime>::iterator it;
	for (it = TradeContext::get_mutable_instance().m_allInstrumentRuntimes.begin();
		it != TradeContext::get_mutable_instance().m_allInstrumentRuntimes.end();
		++it)
	{
		update_ctp_CThostFtdcInvestorPositionField(investor, it->first.c_str(), it->second.investorPositionAllDirections);
	}
}

// 报单
void ctp_redis_worker::update_ctp_CThostFtdcOrderField(const char* investor)
{
	std::map<std::string, InstrumentRuntime>::iterator it;
	for (it = TradeContext::get_mutable_instance().m_allInstrumentRuntimes.begin();
		it != TradeContext::get_mutable_instance().m_allInstrumentRuntimes.end();
		++it)
	{
		update_ctp_CThostFtdcOrderField(ctp_message_def::KEY_ACCOUNT_CTHOSTFTDCORDERFIELD_SUFFIX,investor, it->first.c_str(), it->second.ftdcOrderFields);
		update_ctp_CThostFtdcOrderField(ctp_message_def::KEY_ACCOUNT_PENDING_CTHOSTFTDCORDERFIELD_SUFFIX, investor, it->first.c_str(), it->second.pengdingFtdcOrderFields);
	}
}

// 成交记录
void ctp_redis_worker::update_ctp_CThostFtdcTradeField(const char* investor)
{
	std::map<std::string, InstrumentRuntime>::iterator it;
	for (it = TradeContext::get_mutable_instance().m_allInstrumentRuntimes.begin();
		it != TradeContext::get_mutable_instance().m_allInstrumentRuntimes.end();
		++it)
	{
		update_ctp_CThostFtdcTradeField(investor, it->first.c_str(), it->second.ftdcTradeFields);
	}
}

// 报单错误
void ctp_redis_worker::update_ctp_CThostFtdcInputOrderField(const char* investor)
{
	std::map<std::string, InstrumentRuntime>::iterator it;
	for (it = TradeContext::get_mutable_instance().m_allInstrumentRuntimes.begin();
		it != TradeContext::get_mutable_instance().m_allInstrumentRuntimes.end();
		++it)
	{
		update_ctp_CThostFtdcInputOrderField(investor, it->first.c_str(), it->second.ftdcInputOrderFields);
	}
}

/*只包含四个值*/
void ctp_redis_worker::update_ctp_instrument_value_simple(CThostFtdcDepthMarketDataField *pField, const boost::function<void(const RedisValue &)> &handler)
{
	std::string strkey = boost::str(boost::format("%1%%2%") % pField->InstrumentID %ctp_message_def::KEY_CTP_INSTRUMENT_VALUE_SIMPLE_POSTFIX);
	RedisBuffer key(strkey.c_str());
	
	Json::Value root;
	root["id"] = pField->InstrumentID;
	root["date"] = pField->TradingDay;
	root["open"] = pField->OpenPrice;
	root["high"] = pField->HighestPrice;
	root["low"] = pField->LowestPrice;
	root["close"] = pField->LastPrice;

	std::string strValue = root.toStyledString();
	RedisBuffer rb(strValue);
	redis.command("SET", key, rb,handler);
}


void ctp_redis_worker::get_ctp_instrument_value_simple(const char* instrumentid, const boost::function<void(const RedisValue &)> &handler)
{
	std::string strkey = boost::str(boost::format("%1%%2%") % instrumentid %ctp_message_def::KEY_CTP_INSTRUMENT_VALUE_SIMPLE_POSTFIX);
	RedisBuffer key(strkey.c_str());
	redis.command("GET", key, handler);
}
/*包含所有信息*/
void ctp_redis_worker::update_ctp_instrument_value_full(CThostFtdcDepthMarketDataField *pField)
{

}