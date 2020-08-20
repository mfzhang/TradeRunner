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
        pUserApi = NULL;	//��������ж��Ƿ�����
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

    ///����Ӧ��
    virtual void OnRspError(CThostFtdcRspInfoField* pRspInfo,
                            int nRequestID, bool bIsLast);

    ///���ͻ����뽻�׺�̨ͨ�����ӶϿ�ʱ���÷��������á���������������API���Զ��������ӣ��ͻ��˿ɲ�������
    ///@param nReason ����ԭ��
    ///        0x1001 �����ʧ��
    ///        0x1002 ����дʧ��
    ///        0x2001 ����������ʱ
    ///        0x2002 ��������ʧ��
    ///        0x2003 �յ�������
    virtual void OnFrontDisconnected(int nReason);

    ///������ʱ���档����ʱ��δ�յ�����ʱ���÷��������á�
    ///@param nTimeLapse �����ϴν��ձ��ĵ�ʱ��
    virtual void OnHeartBeatWarning(int nTimeLapse);

    ///���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ�����÷��������á�
    virtual void OnFrontConnected();

    ///��¼������Ӧ
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

    ///��������Ӧ��
    virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

    ///ȡ����������Ӧ��
    virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

    ///�������֪ͨ
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
        //ģ��
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
        // ���ErrorID != 0, ˵���յ��˴������Ӧ
        bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
        if (bResult)
            std::cerr << "--->>> ErrorID=" << pRspInfo->ErrorID
            << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
        return bResult;
    }

	//���½�����֪ͨsubscriber����
	void update_redis_ctp_instrument_marketdata_handler(const RedisValue &root);

	
	/* const���ĳ�2�Σ�������֤�����Ӽ�����ַ*/
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
	//��¼ǰ��
	static const int STATUS_FRONT_CONNECTING = 0X00000010;
	static const int STATUS_FRONT_DISCONNECTED = STATUS_FRONT_CONNECTING + 1;
	static const int STATUS_FRONT_CONNECTION_FAIL = STATUS_FRONT_CONNECTING + 2; // front address

																				 // �û���¼
	static const int STATUS_USER_LOGINING = 0X00000020;
	static const int STATUS_USER_LOGIN_FAIL = STATUS_USER_LOGINING + 1;

	
	int status;
	std::string message;


	boost::mutex pendingInstrumentIds_mutex;
	std::stack<std::string> pendingInstrumentIds;
	RedisClient &publisher;
public:
	//�첽����redis���õ����е�instrument ids
	void get_redis_ctp_instrument_ids_handler(const RedisValue &root);

private:
	int current_ctp_address_index = 0;
};

#endif // QUOTE_H