#include "stdafx.h"

#include <iostream>
#include <boost/asio.hpp>
#include <boost/function.hpp>
//#define ELPP_STL_LOGGING
//#define ELPP_THREAD_SAFE
#define ELPP_THREAD_SAFE  
#include "easylogging++.h"

#include "trade_strategy_argus.h"
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp> 
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <algorithm>
#include "redisclient.h"
#include "strategy_redis_worker.h"
#include "boost/filesystem/path.hpp"  
#include "boost/filesystem/operations.hpp"  
#include "ctp_account_mutex.h"
#include <signal.h>
#include "strategy_redis_command_handler.h"
#include "ctp_message_def.h"
#include "my_strategy_manager.h"
#include "lua_helper.h"
#include "json\json.h"
#include "ctp_account_mutex.h"
#include "redis_heart_beat.h"
#include "historyitem.h"
//#include "HistorySqliteDB.h"
#include "HistoryMysqlDb.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include "kline_redis_worker.h"
#include "strategy_context.h"

INITIALIZE_EASYLOGGINGPP

void stop_application_external(const std::vector<char> &buf);

void kline_value_update_event_handler(const std::vector<char>&buf);

ctp_account_mutex account_mutex;

bool _only_load_strategies = false;
std::string investor_command_channel_name = "";

// asio main 
boost::asio::io_service ioservice;

boost::asio::deadline_timer heartBeatTime(ioservice);

boost::asio::deadline_timer testtimer(ioservice);

// redis main
RedisClient redis(ioservice);

// subscribe redis
RedisClient subscriber(ioservice);

// redis worker
strategy_redis_worker worker(redis);

redis_heart_beat heart_beat(redis,ioservice,2);
// redis command handler
strategy_redis_command_handler redis_cmd_handler(worker);
boost::function<void(const std::vector<char> &msg)> instrument_simple_value_func = boost::bind(&strategy_redis_command_handler::ctp_instrument_simple_value_update_handler, &redis_cmd_handler, _1);

boost::function<void(const std::vector<char> &msg)> local_strategy_update_func = boost::bind(&strategy_redis_command_handler::event_strategy_updated_handler, &redis_cmd_handler, _1);

trade_strategy_argus argus;

void subscriber_connect_handler(bool connected, const std::string& msg);

void mySubscriberErrorHandler(const std::string &s)
{
    LOG(ERROR) << "redis error:" << s;
}

std::vector<RedisAsyncClient::Handle> allHandlers;
/*
* 運行狀態寫入redis
*/

void handle_test_timer(const boost::system::error_code &error)
{
    if (subscriber.isConnected())
    {
        RedisBuffer buffer("rb1805");
        subscriber.publish("rb1805_ins_value_s", buffer);
        testtimer.expires_from_now(boost::posix_time::seconds(10));
        testtimer.async_wait(boost::bind(&handle_test_timer,
            boost::asio::placeholders::error));
    }
}
void handle_heartbeat(const boost::system::error_code & error)
{
	if (redis.isConnected())
	{
        for (int i = 0; i < allHandlers.size(); ++i)
        {
            subscriber.unsubscribe(allHandlers[i]);
        }
	
        allHandlers.clear();

        // 注册所有的合约回调
        std::set<std::string> ins = my_strategy_manager::get_mutable_instance().get_all_scripted_instrumntids();
        for each(std::string insid in ins)
        {

            worker.publish_command(ctp_message_def::SUBSCRIBE_MARKET_DATA, insid.c_str());
            std::string strkey = boost::str(boost::format("%1%%2%") % insid %ctp_message_def::KEY_CTP_INSTRUMENT_VALUE_SIMPLE_POSTFIX);

            // 测试一下吧，所有的策略都绑定到一个回调上
            allHandlers.push_back(subscriber.subscribe(strkey, instrument_simple_value_func));
        }

        worker.update_subscribe_instrument_ids(argus.get_current_investor().c_str());
        //std::string strkey = boost::str(boost::format("%1%%2%") % "rb1705" %ctp_message_def::KEY_CTP_INSTRUMENT_VALUE_SIMPLE_POSTFIX);
        // TO test
        //subscriber.subscribe(strkey, instrument_simple_value_func);
        //strkey = boost::str(boost::format("%1%%2%") % "rb1710" %ctp_message_def::KEY_CTP_INSTRUMENT_VALUE_SIMPLE_POSTFIX);
        // TO test
        //subscriber.subscribe(strkey, instrument_simple_value_func);

        //worker.publish_command(ctp_message_def::SUBSCRIBE_MARKET_DATA, "rb1710");


        // 策略实时修改，回调
        std::string strkey = boost::str(boost::format("%1%%2%%3%") % ctp_message_def::EVENT_SUFFIX %ctp_message_def::LOCAL_STRATEGIES_SUFFIX %my_utils::get_local_ip_address());
        allHandlers.push_back(subscriber.subscribe(strkey, local_strategy_update_func));

        // 策略暫停，回掉
        strkey = boost::str(boost::format("%1%%2%%3%") % ctp_message_def::EVENT_SUFFIX %ctp_message_def::INVESTOR_STRATEGIES_STOP %argus.get_current_investor());
        allHandlers.push_back(subscriber.subscribe(strkey, boost::bind(&stop_application_external, _1)));
        //subscriber.subscribe()

        //细分实时k线数据更新
        allHandlers.push_back(subscriber.subscribe(ctp_message_def::EVENT_KLINE_VALUE_DONE, boost::bind(kline_value_update_event_handler, _1)));

		heartBeatTime.expires_from_now(boost::posix_time::seconds(30));
		heartBeatTime.async_wait(boost::bind(&handle_heartbeat,
			boost::asio::placeholders::error));
	}
}
// method to handle ctrl+c
void siginthandler(int param)
{
	std::cout<<"User pressed Ctrl+C\n"<<std::endl;
	ioservice.stop();
}
void kline_value_retrive_handler(const RedisValue &buf)
{
	std::string content(buf.toString());
	Json::Reader reader;
	Json::Value root;
	Json::Value d_s("");
	Json::Value d_d(0.0f);
	Json::Value d_di(0);
	std::string iid = "";
	double _open, _high, _low, _close;
	int _cycle,_volume;
	if (reader.parse(content, root))
	{
		iid = root.get("id", d_s).asCString();
		if (iid.empty())return;

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

		id = root.get("cycle", d_di);
		if (id == d_di) return;
		_cycle = id.asInt64();

		id = root.get("volume", d_di);
		_volume = id.asInt64();

		std::string datetime = root.get("date", d_s).asCString();

		HistoryItem item;
		item.m_OpenPrice = _open;
		item.m_HighPrice = _high;
		item.m_LowPrice = _low;
		item.m_ClosePrice = _close;
		item.m_InstrumentCode = iid;
		item_code_class item_code= my_utils::get_item_code(iid.c_str());
		item.m_Item = item_code.name+item_code.date;
		item.m_nMinuteDual = _cycle;
		item.m_nVolume = _volume;

		item.m_StartTime = boost::posix_time::time_from_string(datetime);

		LOG(INFO) << "Received k line:(" << item.m_InstrumentCode << "," << item.m_Item << "," << item.m_nMinuteDual<<","<<item.m_StartTime<<":"<< item.m_OpenPrice << "," << item.m_HighPrice << "," << item.m_LowPrice << "," << item.m_ClosePrice << ")";
		//HistorySqliteDB::instance(item.m_Item).getHistoryItems("RB1805", 5);
		//HistorySqliteDB::instance(item.m_Item).CalculateMAIndexType("RB1805", 5, 120);
		//HistorySqliteDB::instance(item.m_Item).RuntimeHistoryItemReceived(item);
		strategy_context::get_mutable_instance().get_history_mysql_db().RuntimeHistoryItemReceived(item);
	}
}
void kline_value_update_event_handler(const std::vector<char>&buf) 
{
	std::string strkey(buf.begin(), buf.end());;
	RedisBuffer key(strkey.c_str());
	redis.command("GET", key,boost::bind(kline_value_retrive_handler,_1));
}
void stop_application_external(const std::vector<char> &buf)
{
	LOG(WARNING) << "Received external stop command, application quit.";

	//subscriber.disconnect();
	//redis.disconnect();
	ioservice.stop();
}

void ctp_local_strategies_result_handler(const RedisValue &buf)
{
	//不更新本地策略
	if (false) {
		my_future_strategy_type external_strategies;
		std::string content(buf.toString());
		Json::Reader reader;
		Json::Value root;
		const char* instruments_key = "instruments";
		if (reader.parse(content, root))
		{
			if (root.isArray())
			{
				for (Json::ArrayIndex  i = 0; i < root.size(); ++i)
				{
					// element的key是instrumentid
					std::map<std::string, std::vector<my_future_strategy>> element;
					if (root[i][instruments_key].isArray())
					{
						// 这一层是instrumentid
						for (Json::ArrayIndex  j = 0; j < root[i][instruments_key].size(); ++j)
						{
							std::vector<my_future_strategy> strategies;
							if (root[i][instruments_key][j]["strategies"].isArray())
							{
								my_future_strategy strategy;
								for (Json::ArrayIndex  k = 0; k < root[i][instruments_key][j]["strategies"].size(); ++k)
								{
									strategy._instrumenId = root[i][instruments_key][j]["instrumentid"].asCString();
									strategy._investor = root[i]["investor"].asCString();
									strategy._finished = root[i][instruments_key][j]["strategies"][k]["finished"].asBool();
									strategy._strategy_script = root[i][instruments_key][j]["strategies"][k]["script"].asCString();
									strategy._buy_count = root[i][instruments_key][j]["strategies"][k]["buy"].asInt();
									strategy._sell_count = root[i][instruments_key][j]["strategies"][k]["sell"].asInt();
								}
							}
							element[instruments_key] = strategies;
						}

					}

					external_strategies[root[i]["investor"].asCString()] = element;
				}
			}
		}


		my_strategy_manager::get_mutable_instance().update_strategies_external(external_strategies);
	}
	//更新完后，策略入库，redis
	worker.refresh_redis_content_ctp_strategy();

	my_strategy_manager::get_mutable_instance().filter_by_investor();

	// another connect as subscribe
	subscriber.connect(boost::asio::ip::address::from_string(argus.get_redis_address()), argus.get_redis_port(), subscriber_connect_handler);


}
// redis asycn connection handler
void subscriber_connect_handler(bool connected, const std::string& msg)
{
	if (!connected)
	{
		LOG(ERROR) << "subscriber connect failed:" << msg<<". Application quit!";
		subscriber.disconnect();
		ioservice.stop();
	}
	else
	{
        subscriber.installErrorHandler(mySubscriberErrorHandler);

        handle_heartbeat(boost::asio::error::basic_errors::fault);
        handle_test_timer(boost::asio::error::basic_errors::fault);
       /* heartBeatTime.expires_from_now(boost::posix_time::seconds(30));
        heartBeatTime.async_wait(boost::bind(&handle_heartbeat,
        	boost::asio::placeholders::error));*/
		
	}
}

// redis asycn connection handler
void redis_connect_handler(bool connected, const std::string& msg)
{
	if (!connected)
	{
		LOG(ERROR) << "redis connect failed:" << msg;
		redis.disconnect();
		ioservice.stop();
	}
	else
	{
		// 这个地方改成先从运行环境读取策略，这样不会影响别的策略的实时数据
		worker.get_local_ctp_strategies(boost::bind(ctp_local_strategies_result_handler, _1));
		
		/*HistoryItem item;
		item.m_nMinuteDual = 5;
		item.m_InstrumentCode = "rb1805";
		item.m_Item = "RB1805";
		item.m_OpenPrice = 1234;
		item.m_HighPrice = 1234;
		item.m_LowPrice = 1234;
		item.m_ClosePrice = 1234;
		item.m_StartTime = boost::posix_time::second_clock::local_time();
		
		kline_redis_worker k_worker(redis);
		k_worker.update_redis_kline_value(item);*/
	}
	// 策略程序不连接交易，下单
}
/*
main method
*/
int  main(int argc, char* argv[])
{
	/*boost::posix_time::ptime now1(boost::posix_time::second_clock::local_time());
	std::string str = boost::str(boost::format("%1% %2$02d:%3$02d:%4$02d") % to_iso_extended_string(now1.date()) % now1.time_of_day().hours() % now1.time_of_day().minutes() % now1.time_of_day().seconds());

	std::cout << str << std::endl;
	boost::posix_time::ptime  pt1(boost::posix_time::time_from_string(str));
	
	str = boost::str(boost::format("%1% %2$02d:%3$02d:%4$02d") % to_iso_extended_string(pt1.date()) % pt1.time_of_day().hours() % pt1.time_of_day().minutes() % pt1.time_of_day().seconds());

	std::cout << str << std::endl;

	return 0;*/
	/*int buycnt;
	int sellcnt;
	lua_helper::invoke_script("D:\\1.lua", false, 1000, 2000, 3000, 2000, "", buycnt, sellcnt);
		return 0;*/
	/*int buyCnt = 0;
	int sellCnt = 0;
	lua_helper::invoke_script("C:\\wodequant\\strategies\\021475\\rb1710\\5days_80p_updown.strategy",
		false, 2444, 2444, 2340, 2344, "20161031", buyCnt, sellCnt);
	std::cout << buyCnt << ":" << sellCnt << std::endl;
	getchar();
	return 0;*/
	signal(SIGINT, siginthandler);
	// 加一个特殊参数--only_loadr
	argus.parse(argc, argv);
	if (!argus.isvalid())
	{
		return -1;
	}

	stringstream ss;
	ss << "C:/wodequant/TradeRunner/logs/" << argus.get_current_investor() << "_tradestrategy_log_%datetime{%Y%M%d}.log";
	el::Configurations conf;
	conf.setToDefault();
	conf.set(el::Level::Global,
		el::ConfigurationType::Filename, ss.str());

	el::Loggers::reconfigureAllLoggers(conf);

	if (!argus.is_only_refresh_strategy())
	{
		account_mutex._path = boost::filesystem::initial_path<boost::filesystem::path>().string();
		account_mutex._path = "C:/wodequant/TradeRunner/instances/";

		account_mutex._file = "strategies_" + argus.get_current_investor();// +TradeContext::get_mutable_instance().argus.get_username();

		// channle name jiushi 文件名
		investor_command_channel_name = account_mutex._file;

		if (!account_mutex.run())
		{
			std::cerr << "Another instance is running, exit!" << std::endl;
			return 0;
		}
	}

	my_strategy_manager::get_mutable_instance().load((std::string(my_utils::K_WODEQUANT_FOLDER) + "strategies\\").c_str());

	if (argus.is_only_refresh_strategy())
	{
		std::cout << my_strategy_manager::get_mutable_instance().serializeStrategies();
		return 0;
	}

	if (!strategy_context::get_mutable_instance().connect_mysql(argus.get_db_connection_string(), argus.get_db_user(), argus.get_db_pwd())) {
		LOG(ERROR) << "mysql database connection failed:" << argus.get_db_connection_string();
		return 0;
	}
	my_strategy_manager::get_mutable_instance().set_current_running_investor(argus.get_current_investor().c_str());
	boost::asio::ip::address address = boost::asio::ip::address::from_string(argus.get_redis_address());

	my_strategy_manager::get_mutable_instance().start_strategy_running();
	// connect to redis
	redis.connect(address, argus.get_redis_port(), redis_connect_handler);

	std::string strkey = boost::str(boost::format("%1%_strategies") % argus.get_current_investor());
	heart_beat.run(strkey);
	ioservice.run();

	my_strategy_manager::get_mutable_instance().stop_strategy_running();
	return 0;
	
}