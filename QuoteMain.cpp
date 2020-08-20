#include "stdafx.h"

#include <iostream>
#include "quote.h"
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
#include "ctp_message_def.h"
#include "KLineGenerator.h"
#include "kline_redis_worker.h"
#include "json\json.h"
#include "HistoryMysqlDb.h"
#include "HistoryItemDBWriter.h"

INITIALIZE_EASYLOGGINGPP

boost::asio::io_service ioservice;
RedisClient redis(ioservice);
RedisClient publisher(ioservice);

// subscribe redis
RedisClient subscriber(ioservice);

ctp_redis_worker worker(redis);
kline_redis_worker k_redis_worker(redis);
Quote quote(ioservice, worker,publisher);
ctp_account_mutex account_mutex;

HistoryMysqlDb history_db;

void subscrbe_instrument_market_data_handler(const std::vector<char> &buf);
void unsubscrbe_instrument_market_data_handler(const std::vector<char> &buf);

std::set<string> item_to_notify;
void ctp_instrument_ids_updated_handler(const std::vector<char> &buf)
{
	worker.get_ctp_instrument_ids(boost::bind(&Quote::get_redis_ctp_instrument_ids_handler, &quote, _1));
}
boost::function<void(const std::vector<char> &msg)> subscrbe_instrument_market_data = boost::bind(&subscrbe_instrument_market_data_handler, _1);
boost::function<void(const std::vector<char> &msg)> unsubscrbe_instrument_market_data = boost::bind(&unsubscrbe_instrument_market_data_handler, _1);

void get_investor_quoted_instrument_ids_handler(const RedisValue &root)
{
	if (root.isString())
	{
		Json::Reader reader;
		Json::Value value;
		if (reader.parse(root.toString(), value))
		{
			if (value.isArray())
			{
				for (int i = 0; i < value.size(); ++i) {
					const char * ins = value[i].asCString();
					LOG(INFO) << "new quoted instrument:" << ins;
					item_to_notify.insert(ins);
				}
			}
		}
	}
}
void get_all_quoted_instrument_ids_handler(const RedisValue &root)
{
	
	if (root.isArray())
	{
		std::vector<RedisValue> vec = root.toArray();

		for (int i = 0; i < vec.size(); ++i) {
			std::string str = vec[i].toString();
			RedisBuffer args(str.c_str());
			redis.command("get", args, get_investor_quoted_instrument_ids_handler);
		}


	}
}
void subscrbe_instrument_market_data_handler(const std::vector<char> &buf)
{
	if (quote.m_bFrontConnected)
	{
		std::string content(buf.begin(), buf.end());
		char* tmp[1] = { new char[256] };
		memset(tmp[0], 0, 256);
		strncpy(tmp[0], content.c_str(), content.length());
		quote.pUserApi->SubscribeMarketData(tmp, 1);

		//这个也添加到item_to_notify中

		LOG(INFO) << "new quoted instrument from subscrbe_instrument_market_data_handler:" << tmp[0];
		item_to_notify.insert(tmp[0]);
	}
}

void unsubscrbe_instrument_market_data_handler(const std::vector<char> &buf)
{
	if (quote.m_bFrontConnected)
	{
		std::string content(buf.begin(), buf.end());

		item_to_notify.insert(content.c_str());
		char* tmp[1] = { new char[256] };
		memset(tmp[0], 0, 256);
		strncpy(tmp[0], content.c_str(), content.length());
		quote.pUserApi->UnSubscribeMarketData(tmp, 1);
	}
}
// method to handle ctrl+c
void siginthandler(int param)
{
	std::cout<<"User pressed Ctrl+C\n"<<std::endl;
	ioservice.stop();
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
		//得到所有的subscribe item
		std::string strkey = boost::str(boost::format("%1%_%2%") % ctp_message_def::SUBSCRIBE_MARKET_DATA  %"*");
		RedisBuffer args(strkey.c_str());
		
		redis.command("keys", strkey, get_all_quoted_instrument_ids_handler);
		quote.Run();
	}
}

// redis asycn connection handler
void publisher_connect_handler(bool connected, const std::string& msg)
{
	if (!connected)
	{
		LOG(ERROR) << "publisher connect failed:" << msg;
		publisher.disconnect();
	}
	else
	{
	}
}

void subscriber_connect_handler(bool connected, const std::string& msg)
{
	if (!connected)
	{
		LOG(ERROR) << "publisher connect failed:" << msg;
		subscriber.disconnect();
	}
	else
	{
		subscriber.subscribe(ctp_message_def::SUBSCRIBE_MARKET_DATA, subscrbe_instrument_market_data);
		subscriber.subscribe(ctp_message_def::UNSUBSCRIBE_MARKET_DATA, unsubscrbe_instrument_market_data);
		subscriber.subscribe(ctp_message_def::KEY_CTP_INSTRUMENT_IDS, boost::bind(&ctp_instrument_ids_updated_handler, _1));
	}
}



/*
main method
*/
int  main(int argc, char* argv[])
{

	signal(SIGINT, siginthandler);

	QuoteRunnerArgus &quote_argus = TradeContext::get_mutable_instance().quote_argus;
	//std::cout<< boost::asio::ip::host_name()<<std::endl;
	quote_argus.parse(argc, argv);
	if (!quote_argus.isvalid())
	{
		return -1;
	}

	account_mutex._path = boost::filesystem::initial_path<boost::filesystem::path>().string();
	account_mutex._path = "C:/wodequant/TradeRunner/instances/";

	account_mutex._file = "quote_query";

	if (!account_mutex.run())
	{
		std::cerr << "Another instance is running, exit!" << std::endl;
		return 0;
	}

	history_db.Connect(quote_argus.get_db_connection_string(), quote_argus.get_db_user(), quote_argus.get_db_pwd());
	
	if (!history_db.is_connected()) {
		LOG(ERROR) << "mysql database connection failed:" << quote_argus.get_db_connection_string();
		return 0;
	}

	HistoryItemDBWriter::StartDaemonWriter();

	KLineGenerator::Initilize("C:/wodequant/parameters/kline_items.txt");
	
	boost::asio::ip::address address = boost::asio::ip::address::from_string(quote_argus.get_redis_address());

	// connect to redis
	redis.connect(address, quote_argus.get_redis_port(), redis_connect_handler);

	// publisher
	publisher.connect(address, quote_argus.get_redis_port(), publisher_connect_handler);

	subscriber.connect(address, quote_argus.get_redis_port(), subscriber_connect_handler);
	
	ioservice.run();

	HistoryItemDBWriter::StopDaemonWriter();
	return 0;
}