/*
** Copyright (C) QPSOFT.COM All rights reserved.
*/
#ifndef QUOTE_H
#define QUOTE_H
#include <iostream>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/function.hpp>
#include "ThostFtdcMdApi.h"
#include "redisasyncclient.h"
#include "redisclient/redisvalue.h"
#include <stack>
#include <boost/thread/mutex.hpp>
//#ifdef WIN32
//#pragma comment(lib, "api/thostmduserapi")
//#endif
class ctp_redis_worker;

class Quote : public CThostFtdcMdSpi
{
public:
    /** Default constructor */
    //Quote(string Broker="2030", string FrontAddr="tcp://asp-sim2-md1.financial-trading-platform.com:26213");
    Quote(boost::asio::io_service &io, ctp_redis_worker &_redis_worker, RedisClient &_publisher) : tradeio(io), actionTimer(io),redis_worker(_redis_worker),publisher(_publisher)
    {
        this->iReqID = 0;
        this->broker = new char[20];
		memset(broker, 0, 20);
        this->frontAddr = new char[256];
		memset(frontAddr, 0, 256);
        pUserApi = NULL;	//方便程序判断是否连接
		user = new char[256];
		memset(user, 0, 256);
		pwd = new char[256];
		memset(pwd, 0, 256);
		m_bFrontConnected = false;
		m_bUserLogin = false;

		

    }

    /** Default destructor */
    virtual ~Quote()
    {
        if (pUserApi)
            pUserApi->Release();

		delete broker;
		delete frontAddr;
		delete user;
		delete pwd;
    }

    ///错误应答
    virtual void OnRspError(CThostFtdcRspInfoField* pRspInfo,
                            int nRequestID, bool bIsLast);

    ///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
    ///@param nReason 错误原因
    ///        0x1001 网络读失败
    ///        0x1002 网络写失败
    ///        0x2001 接收心跳超时
    ///        0x2002 发送心跳失败
    ///        0x2003 收到错误报文
    virtual void OnFrontDisconnected(int nReason);

    ///心跳超时警告。当长时间未收到报文时，该方法被调用。
    ///@param nTimeLapse 距离上次接收报文的时间
    virtual void OnHeartBeatWarning(int nTimeLapse);

    ///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
    virtual void OnFrontConnected();

    ///登录请求响应
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

    ///订阅行情应答
    virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

    ///取消订阅行情应答
    virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

    ///深度行情通知
    virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData);

	void ReqLogin(const char* Investor, const char* PWD)
    {
        CThostFtdcReqUserLoginField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, broker);
        strcpy(req.UserID, Investor);
        strcpy(req.Password, PWD);
        pUserApi->ReqUserLogin(&req, ++iReqID);
    }

	void Run()
	{
		Connect();
	}

	void Connect();
	void ReqConnect(char* addr,char* brokerID);
	
    void ReqConnect(int addrID)
    {
		if(pUserApi == NULL)
		{	
			pUserApi = CThostFtdcMdApi::CreateFtdcMdApi("quote", false);
		}else{
			pUserApi->Release();
			pUserApi = CThostFtdcMdApi::CreateFtdcMdApi("quote", false);
		}
		pUserApi->RegisterSpi(this);
        char* addr1 = new char[256];
        //模拟
        if(addrID == 0)
		{
        	strcpy(this->broker, "2030");
            strcpy(addr1, "tcp://asp-sim2-md1.financial-trading-platform.com:26213");
		}
		else
		{
			strcpy(this->broker, "66666");
			sprintf(addr1, "tcp://ctp1-md%d.citicsf.com:41213", addrID);
		}

        pUserApi->RegisterFront(addr1);
        pUserApi->Init();
		delete addr1;
        //pUserApi->Join();
    }

    CThostFtdcMdApi* pUserApi;
protected:
public:
	char *user;
	char *pwd;

    int iReqID;
    char* broker;
    char* frontAddr;

	bool m_bFrontConnected;
	bool m_bUserLogin;

private:
    bool IsErrorRspInfo(CThostFtdcRspInfoField* pRspInfo)
    {
        // 如果ErrorID != 0, 说明收到了错误的响应
        bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
        if (bResult)
            std::cerr << "--->>> ErrorID=" << pRspInfo->ErrorID
            << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
        return bResult;
    }

	//更新结束后，通知subscriber更新
	void update_redis_ctp_instrument_marketdata_handler(const RedisValue &root);

	
	/* const，改成2次，尽量保证多连接几个地址*/
	static const int MAX_REQUEST_TRY_TIMES = 2;

	boost::asio::io_service &tradeio;
	ctp_redis_worker &redis_worker;
	boost::asio::deadline_timer actionTimer;
	int m_nTryTimes = 0;
	void ResetTryTimes() { m_nTryTimes = 0; }
	void IncreaseTryTimes() { ++m_nTryTimes; };

	void handle_wait_connect(const boost::system::error_code& error);
	void handle_wait_login(const boost::system::error_code& error);
	void Login();

	/** status and message, intertransfer with other application*/
	//登录前置
	static const int STATUS_FRONT_CONNECTING = 0X00000010;
	static const int STATUS_FRONT_DISCONNECTED = STATUS_FRONT_CONNECTING + 1;
	static const int STATUS_FRONT_CONNECTION_FAIL = STATUS_FRONT_CONNECTING + 2; // front address

																				 // 用户登录
	static const int STATUS_USER_LOGINING = 0X00000020;
	static const int STATUS_USER_LOGIN_FAIL = STATUS_USER_LOGINING + 1;

	
	int status;
	std::string message;


	boost::mutex pendingInstrumentIds_mutex;
	std::stack<std::string> pendingInstrumentIds;
	RedisClient &publisher;
public:
	//异步调用redis，得到所有的instrument ids
	void get_redis_ctp_instrument_ids_handler(const RedisValue &root);

private:
	int current_ctp_address_index = 0;
};

#endif // QUOTE_H