//#include "stdafx.h"
//
#define ELPP_THREAD_SAFE  
#include "quote.h"
#include "ThostFtdcUserApiStruct.h"
#include<map>
#include<list>
using namespace std;

#include<boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include<boost/date_time/posix_time/ptime.hpp>

#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp> 
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include "TradeContext.h"
#include "easylogging++.h"
#include <algorithm>
#include "json\json.h"
#include "ctp_redis_worker.h"
#include <boost/thread/lock_guard.hpp>
#include "ctp_message_def.h"
#include "KLineGenerator.h"
//*)
extern std::set<string> item_to_notify;;
void Quote::handle_wait_connect(const boost::system::error_code& error)
{
	if (error) return; // 不是0表示cancel，好像是995
					   // error 还不会用
	if (m_bFrontConnected) return;

	if (m_nTryTimes > MAX_REQUEST_TRY_TIMES)
	{
		// move to next ctp address
		if (current_ctp_address_index < TradeContext::get_mutable_instance().quote_argus.get_addresses().size() - 1)
		{
			++current_ctp_address_index;
			ResetTryTimes();//重置

			LOG(ERROR) << MAX_REQUEST_TRY_TIMES << " times of front connect reached, connecting next address:" << TradeContext::get_mutable_instance().quote_argus.get_addresses()[current_ctp_address_index];
		}
		else
		{
			LOG(ERROR) << MAX_REQUEST_TRY_TIMES << " times of front connect reached";
			actionTimer.get_io_service().stop();
			return;
		}
	}
	LOG(WARNING) << "Connect failed, trying times:" << m_nTryTimes;
	IncreaseTryTimes();

	Connect();
}


void Quote::Connect()
{
	LOG(INFO) << "ReqConnecting, try times = " << m_nTryTimes;
	ReqConnect((char*) TradeContext::get_mutable_instance().quote_argus.get_addresses()[current_ctp_address_index].c_str(),
		(char*) TradeContext::get_mutable_instance().quote_argus.get_broker().c_str());
	actionTimer.expires_from_now(boost::posix_time::seconds(10));
	actionTimer.async_wait(boost::bind(&Quote::handle_wait_connect, this, boost::asio::placeholders::error));
}
//
void Quote::ReqConnect(char* addr, char* brokerID)
{
	strcpy(this->frontAddr, addr);
	strcpy(this->broker, brokerID);
	if (pUserApi == NULL)
	{
		pUserApi = CThostFtdcMdApi::CreateFtdcMdApi("quote", false);
	}
	else {
		pUserApi->Release();
		pUserApi = CThostFtdcMdApi::CreateFtdcMdApi("quote", false);
	}
	pUserApi->RegisterSpi(this);
	pUserApi->RegisterFront(frontAddr);

	pUserApi->Init();
}
//(*行情响应

void Quote::OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Quote::OnFrontConnected()
{
	actionTimer.cancel();
	m_bFrontConnected = true;
	ResetTryTimes();
	this->ReqLogin(user, pwd);
}

void Quote::Login()
{
	ReqLogin(TradeContext::get_mutable_instance().quote_argus.get_username().c_str()
		, TradeContext::get_mutable_instance().quote_argus.get_password().c_str());

	status = STATUS_USER_LOGINING;
	message = boost::str(boost::format("ReqLogining, try times = %1%") % m_nTryTimes);
	//redis_worker.update_ctp_account_login_info(investor, get_trade_status_json_string().c_str());
	LOG(INFO) << message;

	actionTimer.expires_from_now(boost::posix_time::seconds(5));
	actionTimer.async_wait(boost::bind(&Quote::handle_wait_login, this, boost::asio::placeholders::error));
}

void Quote::handle_wait_login(const boost::system::error_code& error)
{
	if (m_bUserLogin) return;

	if (m_nTryTimes > MAX_REQUEST_TRY_TIMES)
	{
		status = STATUS_USER_LOGIN_FAIL;
		message = "User Login failed after 5 try times......";
		actionTimer.get_io_service().stop();
		return;
	}

	IncreaseTryTimes();

	Login();
}

///
void Quote::OnFrontDisconnected(int nReason)
{
	m_bFrontConnected = false;
    LOG(INFO) << "Front disconnected, connecting again!";

    current_ctp_address_index = 0;
    m_nTryTimes = 0;
    Connect();
}

//
void Quote::OnHeartBeatWarning(int nTimeLapse)
{
}

///
void Quote::get_redis_ctp_instrument_ids_handler(const RedisValue &root)
{
	int nret = -1;
	if (root.isString())
	{
		Json::Reader reader;
		Json::Value value;
		if (reader.parse(root.toString(), value))
		{
			if (value.isArray())
			{
				char* tmp[1] = { new char[256] };
				for (Json::ArrayIndex i = 0; i < value.size(); ++i)
				{
					memset(tmp[0], 0, 256);
					const char *ids = value[i].asCString();
					strncpy(tmp[0], ids, strlen(ids));
					//LOG(INFO) << "SubscribeMarketData " << ids;
					nret = pUserApi->SubscribeMarketData(tmp, 1);
					nret = nret;
				}
				delete tmp[0];
			}
		}
	}
}

///
void Quote::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin,
	CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	actionTimer.cancel();
	if (IsErrorRspInfo(pRspInfo))
	{
		m_bUserLogin = false;
	}
	else
	{
		m_bUserLogin = true;
		LOG(INFO) << "login done";

		redis_worker.get_ctp_instrument_ids(boost::bind(&Quote::get_redis_ctp_instrument_ids_handler, this, _1));
		//pUserApi->SubscribeMarketData(nullptr, 0);
		//map<string, CTradeItem>::iterator it;

		//BOOST_FOREACH(mapInstrument::value_type i, dicInstrument)
		/*for (it = TradeItemMgr::instance().m_allTradeItems.begin();
		it != TradeItemMgr::instance().m_allTradeItems.end();
		++it)
		{
		strcpy(tmp[0], it->first.c_str());
		pUserApi->SubscribeMarketData(tmp, 1);
		}*/

	}
}

void Quote::OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (IsErrorRspInfo(pRspInfo))
	{
		//CMyLogger::instance().info(str(boost::format("行情订阅错误:%1%") % pRspInfo->ErrorMsg));
	}
	else
	{

	}
}

void Quote::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}
//*)

//深度行情响应
void Quote::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData)
{
	if(item_to_notify.find(pDepthMarketData->InstrumentID)!= item_to_notify.end())
	{
		boost::lock_guard<boost::mutex> guard(pendingInstrumentIds_mutex);
		pendingInstrumentIds.push(pDepthMarketData->InstrumentID);
	}
	redis_worker.update_ctp_instrument_value_simple(pDepthMarketData,boost::bind(&Quote::update_redis_ctp_instrument_marketdata_handler,this,_1));

	// 生成k线
	std::map<int, KLineGenerator*> gens = KLineGenerator::get(pDepthMarketData->InstrumentID);

	std::map<int, KLineGenerator*>::iterator it;
	for (it = gens.begin(); it != gens.end(); ++it) {
		it->second->handle(pDepthMarketData);
	}
	
}

void Quote::update_redis_ctp_instrument_marketdata_handler(const RedisValue &root)
{
	//std::cout << "update_redis_ctp_instrument_marketdata_handler:" << root.toString();
	std::string id = "";
	{
		boost::lock_guard<boost::mutex> guard(pendingInstrumentIds_mutex);
		if (pendingInstrumentIds.empty()) return;

		id = pendingInstrumentIds.top();
		pendingInstrumentIds.pop();
		if(pendingInstrumentIds.size()>5000)
			std::cout << "pendingInstrumentIds size " << pendingInstrumentIds.size() << std::endl;
	}

	if (id.empty()) return;
	//同时publish两个事件，待测试
	//publisher.publish(ctp_message_def::CHANNEL_INSTRUMENT_VALUE_SIMPLE,id);
	std::string strkey = boost::str(boost::format("%1%%2%") % id %ctp_message_def::KEY_CTP_INSTRUMENT_VALUE_SIMPLE_POSTFIX);
	publisher.publish(strkey,id);
}