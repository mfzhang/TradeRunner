#include "stdafx.h"

#include <boost\asio.hpp>
#include <iostream>
//#define ELPP_STL_LOGGING
//#define ELPP_THREAD_SAFE
#define ELPP_THREAD_SAFE  
#include "easylogging++.h"
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp> 
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <algorithm>
#include "redisclient.h"
#include "boost/filesystem/path.hpp"  
#include "boost/filesystem/operations.hpp"  
#include <signal.h>
#include "ctp_message_def.h"
#include <sstream>
#include <json\json.h>
#include "trade_test_argus.h"
#include "lua_helper.h"

INITIALIZE_EASYLOGGINGPP

std::string investor_command_channel_name = "";
boost::asio::io_service ioservice;
RedisClient redis(ioservice);
RedisClient subscriber(ioservice);

boost::asio::deadline_timer quote_simu_timer(ioservice);
boost::asio::deadline_timer ctp_cmd_simu_timer(ioservice);

trade_test_argus argus;
// method to handle ctrl+c
void siginthandler(int param)
{
	std::cout<<"User pressed Ctrl+C\n"<<std::endl;
	ioservice.stop();
}
std::string instrument = "hc1805";
void subcribe_quote(const boost::system::error_code&error)
{
	quote_simu_timer.async_wait(&subcribe_quote);
	quote_simu_timer.expires_from_now(boost::posix_time::milliseconds(333));

	std::string strkey = boost::str(boost::format("%1%%2%") % instrument %ctp_message_def::KEY_CTP_INSTRUMENT_VALUE_SIMPLE_POSTFIX);

	RedisBuffer key(strkey.c_str());

	Json::Value root;
	root["id"] = instrument;
	root["date"] = "";
	root["open"] = 3880;
	root["high"] = 3880;
	root["low"] = 3880;
	root["close"] = 3880;

	std::string strValue = root.toStyledString();
	std::cout << strValue << std::endl;
	RedisBuffer rb(strValue);
	redis.command("SET", key, rb);

	redis.publish(strkey, instrument);
}

int buycnt = -1;
int sellcnt = -1;
void subcribe_ctp_cmd(const boost::system::error_code&error)
{
	ctp_cmd_simu_timer.async_wait(&subcribe_ctp_cmd);
	ctp_cmd_simu_timer.expires_from_now(boost::posix_time::milliseconds(400));

	/*std::stringstream ss;
	ss << "ensure_ins|hc805,1," << buycnt << ";hc1805,-1," << sellcnt;

	std::cout << ss.str() << std::endl;*/
	/*++buycnt;
	--sellcnt;

	if (buycnt > 10)buycnt = 0;
	if (sellcnt < 0)sellcnt = 10;*/

	
	redis.publish("investor_"+argus.get_username(), argus.get_strategy_simu());
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
		if (argus.is_quote_simu()) {
			//启动 模拟timer
			quote_simu_timer.async_wait(&subcribe_quote);
			quote_simu_timer.expires_from_now(boost::posix_time::milliseconds(333));
		}

		if (!argus.get_strategy_simu().empty()) {
			ctp_cmd_simu_timer.async_wait(&subcribe_ctp_cmd);
			ctp_cmd_simu_timer.expires_from_now(boost::posix_time::milliseconds(400));
		}
	}
}



/*
main method
*/
int  main(int argc, char* argv[])
{
	signal(SIGINT, siginthandler);

	argus.parse(argc, argv);

	if (!argus.get_strategy_simu().empty()
		|| argus.is_quote_simu())
	{
		boost::asio::ip::address address = boost::asio::ip::address::from_string(argus.get_redis_address());

		// connect to redis
		redis.connect(address, argus.get_redis_port(), redis_connect_handler);

		ioservice.run();
	}
	else if (!argus.get_lua_filename().empty()) {
		//只验证文件是否能编译通过，逻辑没法检查
		std::string content = my_utils::get_file_content(argus.get_lua_filename().c_str());
		my_utils::replace_all_distinct(content, "{{INVESTOR}}", argus.get_username());
		my_utils::replace_all_distinct(content, "{{INSTRUMENT}}", "rb1805");
		my_utils::replace_all_distinct(content, "{{ITEM}}", "RB1805");
		for (int i = 0; i < 10; ++i) {
			std::stringstream ss;
			ss << "{{" << i << "}}";
			my_utils::replace_all_distinct(content, ss.str(), "10");
		}
		lua_helper::invoke_script(content,false, 1000, 2000, 3000, 2000, "fakdedate", buycnt, sellcnt);
		std::cout << "buycnt =" << buycnt << ";sellcnt=" << sellcnt << std::endl;
	}
	return 0;
}