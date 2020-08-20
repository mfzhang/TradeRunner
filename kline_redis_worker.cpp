#include "stdafx.h"
#include "kline_redis_worker.h"
#include "HistoryItem.h"
#include "json\json.h"
#include <boost/format.hpp>
#include "ctp_message_def.h"
#include <boost/date_time/posix_time/posix_time.hpp>

kline_redis_worker::kline_redis_worker(RedisClient &_redis):redis(_redis){

}
kline_redis_worker::~kline_redis_worker()
{

}

void kline_redis_worker::update_redis_kline_value(HistoryItem &_item)
{
	std::string strkey = boost::str(boost::format("%1%%2%%3%") % _item.m_Item %ctp_message_def::KEY_KLINE_REDIS_VALUE %_item.m_nMinuteDual);
	RedisBuffer key(strkey.c_str());

	Json::Value root;
	root["id"] = _item.m_InstrumentCode;//这个地方用的是ctp的代码，暂时用不到，两个代码真的有点乱
	root["date"] = boost::str(boost::format("%1% %2$02d:%3$02d:%4$02d") % to_iso_extended_string(_item.m_StartTime.date()) % _item.m_StartTime.time_of_day().hours() % _item.m_StartTime.time_of_day().minutes() % _item.m_StartTime.time_of_day().seconds());;
	root["open"] = _item.m_OpenPrice;
	root["high"] = _item.m_HighPrice;
	root["low"] = _item.m_LowPrice;
	root["close"] = _item.m_ClosePrice;
	root["volume"] = _item.m_nVolume;
	root["cycle"] = _item.m_nMinuteDual;

	std::string strValue = root.toStyledString();
	RedisBuffer rb(strValue);
	redis.command("SET", key, rb);
}