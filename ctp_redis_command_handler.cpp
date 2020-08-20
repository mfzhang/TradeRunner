#include "ctp_redis_command_handler.h"
#include "ctp_command_handler.h"
#include "TradeContext.h"
#include <json\json.h>
#include "ctp_message_def.h"

static boost::posix_time::ptime last_subscribe_time(boost::posix_time::second_clock::local_time());
static std::map<std::string, RedisAsyncClient::Handle> subscribedInstruments;
ctp_redis_command_handler::ctp_redis_command_handler(ctp_command_handler &handler, ctp_redis_worker &redis_worker, RedisClient &_cli):_handler(handler),_redis_worker(redis_worker),subscriber(_cli)
{
}
ctp_redis_command_handler::~ctp_redis_command_handler()
{
}
void ctp_redis_command_handler::execute(const std::vector<char> &buf)
{
	std::string content(buf.begin(), buf.end());
	ctp_command _cmd;
	if (_parser.parse(content, _cmd))
	{
        boost::posix_time::ptime now(boost::posix_time::second_clock::local_time());
        boost::posix_time::time_duration td = now - last_subscribe_time;
        if (td.total_seconds() >= 30 
            && subscribedInstruments.size()>0) {//半分钟刷新一次
            std::map<std::string, RedisAsyncClient::Handle>::iterator it;
            for (it = subscribedInstruments.begin(); it != subscribedInstruments.end(); ++it)
            {
                subscriber.unsubscribe(it->second);
            }
            subscribedInstruments.clear();
        }

        if (subscribedInstruments.size() <= 0)
        {   
            for (size_t i = 0; i < _cmd._allArgus.size(); ++i) {
                //if (subscribedInstruments[_cmd._allArgus[i]._instrumentId] == 1)continue;
                if (subscribedInstruments.find(_cmd._allArgus[i]._instrumentId) != subscribedInstruments.end()) continue;

                //否则需要注册
                std::string strkey = boost::str(boost::format("%1%%2%") % _cmd._allArgus[i]._instrumentId  %ctp_message_def::KEY_CTP_INSTRUMENT_VALUE_SIMPLE_POSTFIX);

                // 测试一下吧，所有的策略都绑定到一个回调上
                subscribedInstruments[_cmd._allArgus[i]._instrumentId] = subscriber.subscribe(strkey, boost::bind(&ctp_redis_command_handler::ctp_instrument_simple_value_update_handler, this, _1));
            }

            last_subscribe_time = now;
        }
        

		_handler.execute(_cmd);
	}
}

void ctp_redis_command_handler::ctp_instrument_simple_value_update_handler(const std::vector<char> &buf)
{
	std::string content(buf.begin(), buf.end());
	_redis_worker.get_ctp_instrument_value_simple(content.c_str(),boost::bind(&ctp_redis_command_handler::ctp_instrument_simple_value_result_handler,this,_1));
}

void ctp_redis_command_handler::ctp_instrument_simple_value_result_handler(const RedisValue &buf)
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

		std::map<std::string, InstrumentRuntime> & ins = TradeContext::get_mutable_instance().m_allInstrumentRuntimes;
		if (ins.find(iid) == ins.end()) return;
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

		TradeContext::get_mutable_instance().m_allInstrumentRuntimes[iid].latestValue.open = _open;
		TradeContext::get_mutable_instance().m_allInstrumentRuntimes[iid].latestValue.close = _close;
		TradeContext::get_mutable_instance().m_allInstrumentRuntimes[iid].latestValue.high = _high;
		TradeContext::get_mutable_instance().m_allInstrumentRuntimes[iid].latestValue.low = _low;
		TradeContext::get_mutable_instance().m_allInstrumentRuntimes[iid].latestValue.tradingDay = _tradingDay;
	}
}