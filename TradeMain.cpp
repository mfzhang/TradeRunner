#include "stdafx.h"

#include <iostream>
#include "trade.h"
#include "ctp_command_handler.h"

//#define ELPP_STL_LOGGING
//#define ELPP_THREAD_SAFE
#define ELPP_THREAD_SAFE  
#include "easylogging++.h"




#include "TradeContext.h"
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp> 
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <algorithm>
#include "redisclient.h"
#include "ctp_redis_worker.h"
#include "boost/filesystem/path.hpp"  
#include "boost/filesystem/operations.hpp"  
#include "ctp_account_mutex.h"
#include <signal.h>
#include "ctp_redis_command_handler.h"
#include "ctp_message_def.h"
#include "redis_heart_beat.h"
#include <sstream>
INITIALIZE_EASYLOGGINGPP

std::string investor_command_channel_name = "";
boost::asio::io_service ioservice;
RedisClient redis(ioservice);
RedisClient subscriber(ioservice);

ctp_redis_worker worker(redis);
Trade trade(ioservice,worker,my_utils::get_current_guid().c_str());
ctp_account_mutex account_mutex;

redis_heart_beat heart_beat(redis, ioservice, 3);

boost::asio::deadline_timer heartBeatTime(ioservice);

ctp_command_handler ctp_handler(trade);
ctp_redis_command_handler redis_cmd_handler(ctp_handler,worker,subscriber);
boost::function<void(const std::vector<char> &msg)> redis_command_func = boost::bind(&ctp_redis_command_handler::execute, &redis_cmd_handler, _1);
boost::function<void(const std::vector<char> &msg)> instrument_simple_value_func = boost::bind(&ctp_redis_command_handler::ctp_instrument_simple_value_update_handler, &redis_cmd_handler, _1);

RedisAsyncClient::Handle handle1;
RedisAsyncClient::Handle handle2;
RedisAsyncClient::Handle handle3;

void redis_connect_handler(bool connected, const std::string& msg);
void subscriber_connect_handler(bool connected, const std::string& msg);
void myRedisErrorHandler(const std::string &s);
void mySubscriberErrorHandler(const std::string &s);
void stop_application_external(const std::vector<char> &buf);



void handle_heartbeat(const boost::system::error_code & error)
{
    if (subscriber.isConnected())
    {
        subscriber.unsubscribe(handle1);
        subscriber.unsubscribe(handle3);

        
        handle1 = subscriber.subscribe(investor_command_channel_name, redis_command_func);
        //LOG(ERROR) << "重新订阅redis事件"<<handle1.id<<"  "<<handle1.channel;
        std::string strTradeStopKey = boost::str(boost::format("%1%%2%%3%") % ctp_message_def::EVENT_SUFFIX %ctp_message_def::INVESTOR_TRADEAPP_STOP %TradeContext::get_mutable_instance().argus.get_username());
        // 策略暫停，回掉
        handle3 = subscriber.subscribe(strTradeStopKey, boost::bind(&stop_application_external, _1));
        //LOG(ERROR) << "重新订阅redis事件" << handle3.id << "  " << handle3.channel;
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

std::vector<string>keys_to_delete;
void delete_next_key(const RedisValue &root)
{
	if (root.isError())
	{
		std::cout << root.toString() << std::endl;
	}
	else {
		keys_to_delete.erase(keys_to_delete.begin());
		std::cout << "delete done, move to next one" << std::endl;
	}

	if (!keys_to_delete.empty())
	{
		RedisBuffer rb(keys_to_delete[0]);
		std::cout << "delete redis key:" << keys_to_delete[0] << std::endl;
		redis.command("del", rb, boost::bind(&delete_next_key, _1));
	}
	else {
		trade.Run();
	}
}
void delete_keys(const RedisValue &root)
{

	if (root.isArray())
	{
		std::vector<RedisValue> arr = root.toArray();
		for (size_t i = 0; i < arr.size(); ++i)
		{
			keys_to_delete.push_back(arr[i].toString());

		}
	}
	else
	{
		keys_to_delete.push_back(root.toString());
	}

	if (!keys_to_delete.empty())
	{
		RedisBuffer rb(keys_to_delete[0]);
		std::cout << "delete redis key:" << keys_to_delete[0] << std::endl;
		redis.command("del", rb, boost::bind(&delete_next_key, _1));
	}
	else {
		trade.Run();
	}
}

void myRedisErrorHandler(const std::string &s)
{
	std::cout << s << std::endl;

	boost::asio::ip::address address = boost::asio::ip::address::from_string(TradeContext::get_mutable_instance().argus.get_redis_address());
	redis.disconnect();
	// connect to redis
	redis.connect(address, TradeContext::get_mutable_instance().argus.get_redis_port(), redis_connect_handler);
}

void mySubscriberErrorHandler(const std::string &s)
{
	std::cout << s << std::endl;


	boost::asio::ip::address address = boost::asio::ip::address::from_string(TradeContext::get_mutable_instance().argus.get_redis_address());
	subscriber.disconnect();
	// another connect as subscribe
	subscriber.connect(address, TradeContext::get_mutable_instance().argus.get_redis_port(), subscriber_connect_handler);
}

// redis asycn connection handler
void redis_connect_handler(bool connected, const std::string& msg)
{
	if (!connected)
	{
		LOG(ERROR) << "redis connect failed:" << msg << ". Application quit!";
		redis.disconnect();
		ioservice.stop();
	}
	else
	{
		
		//每次启动清空历史数据
		std::string strkey = boost::str(boost::format("%1%_*") %TradeContext::get_mutable_instance().argus.get_username());
		RedisBuffer rb(strkey);
		redis.command("keys",rb, boost::bind(&delete_keys,_1));
		//trade.Run();
	}
}



void stop_application_external(const std::vector<char> &buf)
{
	LOG(WARNING) << "Received external stop command, application quit.";
	subscriber.unsubscribe(handle1);
	//subscriber.unsubscribe(handle2);
	subscriber.unsubscribe(handle3);
	ioservice.stop();
	//redis.disconnect();
	//subscriber.disconnect();
	
}

// redis asycn connection handler
void subscriber_connect_handler(bool connected, const std::string& msg)
{
	if (!connected)
	{
		LOG(ERROR) << "subscriber connect failed:" << msg;
		subscriber.disconnect();
	}
	else
	{
        redis.installErrorHandler(&myRedisErrorHandler);
        
  //      handle1 = subscriber.subscribe(investor_command_channel_name, redis_command_func);
  //      //LOG(ERROR) << "重新订阅redis事件"<<handle1.id<<"  "<<handle1.channel;
  //      std::string strTradeStopKey = boost::str(boost::format("%1%%2%%3%") % ctp_message_def::EVENT_SUFFIX %ctp_message_def::INVESTOR_TRADEAPP_STOP %TradeContext::get_mutable_instance().argus.get_username());
  //      // 策略暫停，回掉
  //      handle3 = subscriber.subscribe(strTradeStopKey, boost::bind(&stop_application_external, _1));

		//heartBeatTime.expires_from_now(boost::posix_time::seconds(30));
  //      heartBeatTime.async_wait(boost::bind(&handle_heartbeat,
  //          boost::asio::placeholders::error));

        handle_heartbeat(boost::asio::error::basic_errors::fault);
	}
}
/*
main method
*/
int  main(int argc, char* argv[])
{
	

	signal(SIGINT, siginthandler);
	//std::cout<< boost::asio::ip::host_name()<<std::endl;
	TradeContext::get_mutable_instance().argus.parse(argc, argv);
	if (!TradeContext::get_mutable_instance().argus.isvalid())
	{
		return -1;
	}

	stringstream ss;
	ss << "C:/wodequant/TradeRunner/logs/" << TradeContext::get_mutable_instance().argus.get_username() << "_traderunner_log_%datetime{%Y%M%d}.log";
	el::Configurations conf;
	conf.setToDefault();
	conf.set(el::Level::Global,
		el::ConfigurationType::Filename,ss.str());

	el::Loggers::reconfigureAllLoggers(conf);

	account_mutex._path = boost::filesystem::initial_path<boost::filesystem::path>().string();
	account_mutex._path = "C:/wodequant/TradeRunner/instances/";

	account_mutex._file = "investor_" + TradeContext::get_mutable_instance().argus.get_username();

	// channle name jiushi 文件名
	investor_command_channel_name = account_mutex._file;

	if (!account_mutex.run())
	{
		std::cerr << "Another instance is running, exit!" << std::endl;
		return 0;
	}

	boost::asio::ip::address address = boost::asio::ip::address::from_string(TradeContext::get_mutable_instance().argus.get_redis_address());

	// connect to redis
	redis.connect(address, TradeContext::get_mutable_instance().argus.get_redis_port(), redis_connect_handler);

	// another connect as subscribe
	subscriber.connect(address, TradeContext::get_mutable_instance().argus.get_redis_port(), subscriber_connect_handler);

	//trade.investorPostionQueryCallback = boost::bind(&ctp_command_handler::run, &redis_cmd_handler._handler);
	std::string strkey = boost::str(boost::format("%1%_trade") % TradeContext::get_mutable_instance().argus.get_username());
	heart_beat.run(strkey);
	ioservice.run();
	return 0;
}