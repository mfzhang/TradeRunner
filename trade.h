/*
** Copyright (C) QPSOFT.COM All rights reserved.
*/
#ifndef TRADE_H
#define TRADE_H

#include "ThostFtdcTraderApi.h"
#include <iostream>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
using namespace std;
using namespace boost::posix_time;
class ctp_redis_worker;
typedef enum {
	TRADEQUEUEITEM_OPENCLOSE = 0, //open
	TRADEQUEUEITEM_QUERY_POSITION, //��ѯ��������
	TRADEQUEUEITEM_QUERY_ACCOUNT, //��ѯ�˻�
	TRADEQUEUEITEM_CANCELEORDER//ȡ���ҵ�
} TRADEITEM_TYPE;
class TradeQuqueItem 
{
protected:
	TRADEITEM_TYPE _type;
};

class TradeQuqueItemOpenClose:public TradeQuqueItem 
{
public:
	TradeQuqueItemOpenClose(const char * ins, double price, int director,int offset,int volume) {

		_type = TRADEQUEUEITEM_OPENCLOSE;
		_instrument = ins;
		_price = price;
		_director = director;
		_offset = offset;
		_volume = volume;
	}
	inline bool operator == (const TradeQuqueItemOpenClose &r) const {
		return _instrument.compare(r._instrument) == 0
			&& _price == r._price
			&& _director == r._director
			&& _volume==r._volume;
	}
public:
	std::string _instrument;
	double _price;
	int _director;
	int _offset; 
	int _volume;

	std::string _orderref;
};


class TradeQuqueItemQueryPosition :public TradeQuqueItem
{
public:
	TradeQuqueItemQueryPosition(const char * ins) {

		_type = TRADEQUEUEITEM_QUERY_POSITION;
		_instrument = ins;
	}
	inline bool operator == (const TradeQuqueItemQueryPosition &r) const {
		return _instrument.compare(r._instrument) == 0;
	}
public:
	std::string _instrument;
};

class TradeQuqueItemQueryAccount :public TradeQuqueItem
{
public:
	TradeQuqueItemQueryAccount(const char * investor) {

		_type = TRADEQUEUEITEM_QUERY_ACCOUNT;
		_investor = investor;
	}

public:
	std::string _investor;
};

class TradeQuqueItemCancelOrder :public TradeQuqueItem
{
public:
	TradeQuqueItemCancelOrder(const char * ins,int session,int front,const char * orderref) {
		_type = TRADEQUEUEITEM_CANCELEORDER;
		_instrument = ins;
		_session = session;
		_frontid = front;
		_orderref = orderref;
	}
	inline bool operator == (const TradeQuqueItemCancelOrder &r) const{
		return _instrument.compare(r._instrument) == 0
			&& _session == r._session
			&& _frontid == r._frontid
			&& _orderref.compare(r._orderref)==0;
	}
public:
	std::string _instrument;
	int _session;
	int _frontid;
	std::string _orderref;
};

class Trade : public CThostFtdcTraderSpi
{
public:
	//�о� ctp��ͬ������ģ����֮ǰ���¼�û�д����꣬����Ķ�������
	//���Լ�һ��������ʾ��ǰ�Ƿ���request���ڱ�����
	bool m_bRequestInProgress = false;
	//�����ʾ�Ƿ��¼���̽�����Ҳ����˵������¼����ѯ���еģ�
	//������Ϊ�Ƿ����ߵı�ע
	bool m_bCompleteLoginProgress = false;
	//���׽�����Ϊ׼ȷ����ֲ��������²�ѯһ��
	bool m_bQryInverstorPositionAfterTrade = false;
	/* const*/
	static const int MAX_REQUEST_TRY_TIMES = 10;
	/* boost asio*/
	boost::asio::io_service &tradeio;
private:
	int m_nTryTimes = 0;
	//void ResetTryTimes() { 
	//	//ֻ�е�û�в�ѯҵ���ʱ�����
	//	if(!m_bRequestInProgress)
	//		m_nTryTimes = 0; 
	//}
	//void IncreaseTryTimes() { ++m_nTryTimes; };
	

	/*******connect************/
	void handle_wait_connect(const boost::system::error_code& error);
	boost::asio::deadline_timer *frontConnectTime;
	void Connect();
	/****************************/

	/*******Login************/
	void handle_wait_login(const boost::system::error_code& error);
	boost::asio::deadline_timer *loginTime;
	void Login();
	/****************************/

	/*******��ѯ����ȷ��************/
	void handle_wait_qry_settlementinfo_confirm(const boost::system::error_code& error);
	boost::asio::deadline_timer *qrysettlementInfoConfirmTime;
	void ReqQrySettlementInfoConfirm();

	/*******ReqSettlementInfoConfirm************/
	void handle_wait_settlementinfo_confirm(const boost::system::error_code& error);
	boost::asio::deadline_timer *settlementInfoConfirmTime;
	void ReqSettlementInfoConfirm();

	/*******ReqQrySettlementInfo************/
	void handle_wait_qrysettlementinfo(const boost::system::error_code& error);
	boost::asio::deadline_timer *qrySettlementInfoTime;
	void ReqQrySettlementInfo(); 
	/****************************/

	/*******���Լ************/
	void handle_wait_qryinstrument(const boost::system::error_code& error);
	boost::asio::deadline_timer *qryInstrumentTime;
	void ReqQryInstrument();
	/****************************/

	/*******��ֲ�************/
	boost::asio::deadline_timer queueQryinvestorpositionTimer;
	void handle_queue_qryinvestorposition(const boost::system::error_code& error);
	void handle_wait_qryinvestorposition(const boost::system::error_code& error, const char* _instrumentId);
	boost::asio::deadline_timer *qryInvestorPositionTime;
	// ��־��ǰ���ڲ�ѯ�ĺ�Լ����Щʱ����Ҫ��ѯ���еĺ�Լ
	std::string m_currentQryInstrumentId;
	virtual void ReqQryInvestorPosition();
	virtual void ReqQryInvestorPosition(const char *_instrumentID);
	/****************************/

	/*******�ҵ�************/
	void handle_wait_orderinsert(const boost::system::error_code& error, char ordertype,const char* _instrumentId, double price, int director, int offset, int volume);
	boost::asio::deadline_timer *orderinsertTime;
	virtual int ReqOrderInsert(const char* instrument, double price, int director, int offset, int volume);
	virtual int ReqOrderInsert(const char* instrument, int director, int offset, int volume);
	
	/*******************/

	/*******�������޸���ʱ��֧��************/
	void handle_wait_orderaction(const boost::system::error_code& error, const char* instrument, int session, int frontid, const char* orderref);
	boost::asio::deadline_timer *orderactionTime;
	virtual void ReqOrderAction(const char* instrument, int session, int frontid, const char* orderref);
	/*******************/

	// other timer
	// 1���Ӹ���һ��redis������״̬
	boost::asio::deadline_timer tradeStatusUpdateTimer;
	/*******connect************/
	void handle_wait_status_update(const boost::system::error_code& error);

	ctp_redis_worker &redis_worker;
public:
	//�����ͳ������ⲿ����
	bool ReqOrderInsertExternal(const char* instrument, double price, int director, int offset, int volume);
	bool ReqOrderInsertExternal(const char* instrument, int director, int offset, int volume);
	bool ReqOrderActionExternal(const char* instrument, int session, int frontid, const char* orderref);
	boost::function<void()> investorPostionQueryCallback;
public:
	/*********run *******/
	void Run();
	/*********************/

	/** Default constructor */
	Trade(boost::asio::io_service &io, ctp_redis_worker &_redis_worker,const char *guid): 
		tradeio(io), 
		tradeStatusUpdateTimer(io), 
		redis_worker(_redis_worker), 
		queueQryinvestorpositionTimer(io),
		current_guid(guid)
	{

		iReqID = 0;
		addrID = -1;
		investor = new char[256];
		memset(investor, 0, 256);
		broker = new char[256];
		memset(broker, 0, 256);
		frontAddr = new char[256];
		memset(frontAddr, 0, 256);
		settleInfo = "";
		pUserApi = NULL;	//��������ж��Ƿ�����


		m_bFrontConnected = false;
		m_bUserLogin = false;


		tradingDay = new char[9];
		memset(tradingDay, 0, 9);
		// ��¼��ǰ������orderref��ʼֵ��ͨ����ֵ�ж��Ƿ��ǵ�ǰ���̷����ģ�ÿ���õ�ǰ��ʱ��+10000
		// �÷�����ʱֻ�ܼ��ٳ�ͻ�������ܱ�֤û�г�ͻ
		orderRefInitValue = (int) time(nullptr) * 10000;
		orderRef = orderRefInitValue;

		orderactionTime 
			= orderinsertTime
			= qryInvestorPositionTime
			= qryInstrumentTime
			= qrysettlementInfoConfirmTime
			= qrySettlementInfoTime
			= loginTime 
			= frontConnectTime 
			= settlementInfoConfirmTime 
			= new boost::asio::deadline_timer(io);//�ȶ���һ������ʾһ��ʱ��ֻ����һ����ѯ����
	}

	/** Default destructor */
	virtual ~Trade()
	{
		if (pUserApi)
			pUserApi->Release();

		delete broker;
		delete frontAddr;
		delete investor;
		delete tradingDay;

		delete frontConnectTime;
	}

	///���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ�����÷��������á�
	virtual void OnFrontConnected();

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

	///�ͻ�����֤��Ӧ
	virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);


	///��¼������Ӧ
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�ǳ�������Ӧ
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�û��������������Ӧ
	virtual void OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField* pUserPasswordUpdate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�ʽ��˻��������������Ӧ
	virtual void OnRspTradingAccountPasswordUpdate(CThostFtdcTradingAccountPasswordUpdateField* pTradingAccountPasswordUpdate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///����¼��������Ӧ
	virtual void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///Ԥ��¼��������Ӧ
	virtual void OnRspParkedOrderInsert(CThostFtdcParkedOrderField* pParkedOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///Ԥ�񳷵�¼��������Ӧ
	virtual void OnRspParkedOrderAction(CThostFtdcParkedOrderActionField* pParkedOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///��������������Ӧ
	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///��ѯ��󱨵�������Ӧ
	virtual void OnRspQueryMaxOrderVolume(CThostFtdcQueryMaxOrderVolumeField* pQueryMaxOrderVolume, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///Ͷ���߽�����ȷ����Ӧ
	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///ɾ��Ԥ����Ӧ
	virtual void OnRspRemoveParkedOrder(CThostFtdcRemoveParkedOrderField* pRemoveParkedOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///ɾ��Ԥ�񳷵���Ӧ
	virtual void OnRspRemoveParkedOrderAction(CThostFtdcRemoveParkedOrderActionField* pRemoveParkedOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯ������Ӧ
	virtual void OnRspQryOrder(CThostFtdcOrderField* pOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯ�ɽ���Ӧ
	virtual void OnRspQryTrade(CThostFtdcTradeField* pTrade, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯͶ���ֲ߳���Ӧ
	virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯ�ʽ��˻���Ӧ
	virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField* pTradingAccount, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯͶ������Ӧ
	virtual void OnRspQryInvestor(CThostFtdcInvestorField* pInvestor, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯ���ױ�����Ӧ
	virtual void OnRspQryTradingCode(CThostFtdcTradingCodeField* pTradingCode, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯ��Լ��֤������Ӧ
	virtual void OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField* pInstrumentMarginRate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯ��Լ����������Ӧ
	virtual void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField* pInstrumentCommissionRate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯ��������Ӧ
	virtual void OnRspQryExchange(CThostFtdcExchangeField* pExchange, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯ��Լ��Ӧ
	virtual void OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯ������Ӧ
	virtual void OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯͶ���߽�������Ӧ
	virtual void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField* pSettlementInfo, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯת��������Ӧ
	virtual void OnRspQryTransferBank(CThostFtdcTransferBankField* pTransferBank, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯͶ���ֲ߳���ϸ��Ӧ
	virtual void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField* pInvestorPositionDetail, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯ�ͻ�֪ͨ��Ӧ
	virtual void OnRspQryNotice(CThostFtdcNoticeField* pNotice, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯ������Ϣȷ����Ӧ
	virtual void OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯͶ���ֲ߳���ϸ��Ӧ
	virtual void OnRspQryInvestorPositionCombineDetail(CThostFtdcInvestorPositionCombineDetailField* pInvestorPositionCombineDetail, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///��ѯ��֤����ϵͳ���͹�˾�ʽ��˻���Կ��Ӧ
	virtual void OnRspQryCFMMCTradingAccountKey(CThostFtdcCFMMCTradingAccountKeyField* pCFMMCTradingAccountKey, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯ�ֵ��۵���Ϣ��Ӧ
	virtual void OnRspQryEWarrantOffset(CThostFtdcEWarrantOffsetField* pEWarrantOffset, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯת����ˮ��Ӧ
	virtual void OnRspQryTransferSerial(CThostFtdcTransferSerialField* pTransferSerial, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯ����ǩԼ��ϵ��Ӧ
	//virtual void OnRspQryAccountregister(CThostFtdcAccountregisterField* pAccountregister, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);
	///�����ѯ����ǩԼ��ϵ��Ӧ
	virtual void OnRspQryAccountregister(CThostFtdcAccountregisterField *pAccountregister, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	///����Ӧ��
	virtual void OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///����֪ͨ
	virtual void OnRtnOrder(CThostFtdcOrderField* pOrder);

	///�ɽ�֪ͨ
	virtual void OnRtnTrade(CThostFtdcTradeField* pTrade);

	///����¼�����ر�
	virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo);

	///������������ر�
	virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField* pOrderAction, CThostFtdcRspInfoField* pRspInfo);

	///��Լ����״̬֪ͨ
	virtual void OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField* pInstrumentStatus);

	///����֪ͨ
	virtual void OnRtnTradingNotice(CThostFtdcTradingNoticeInfoField* pTradingNoticeInfo);

	///��ʾ������У�����
	virtual void OnRtnErrorConditionalOrder(CThostFtdcErrorConditionalOrderField* pErrorConditionalOrder);

	///�����ѯǩԼ������Ӧ
	virtual void OnRspQryContractBank(CThostFtdcContractBankField* pContractBank, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯԤ����Ӧ
	virtual void OnRspQryParkedOrder(CThostFtdcParkedOrderField* pParkedOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯԤ�񳷵���Ӧ
	virtual void OnRspQryParkedOrderAction(CThostFtdcParkedOrderActionField* pParkedOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯ����֪ͨ��Ӧ
	virtual void OnRspQryTradingNotice(CThostFtdcTradingNoticeField* pTradingNotice, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯ���͹�˾���ײ�����Ӧ
	virtual void OnRspQryBrokerTradingParams(CThostFtdcBrokerTradingParamsField* pBrokerTradingParams, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯ���͹�˾�����㷨��Ӧ
	virtual void OnRspQryBrokerTradingAlgos(CThostFtdcBrokerTradingAlgosField* pBrokerTradingAlgos, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///���з��������ʽ�ת�ڻ�֪ͨ
	virtual void OnRtnFromBankToFutureByBank(CThostFtdcRspTransferField* pRspTransfer);

	///���з����ڻ��ʽ�ת����֪ͨ
	virtual void OnRtnFromFutureToBankByBank(CThostFtdcRspTransferField* pRspTransfer);

	///���з����������ת�ڻ�֪ͨ
	virtual void OnRtnRepealFromBankToFutureByBank(CThostFtdcRspRepealField* pRspRepeal);

	///���з�������ڻ�ת����֪ͨ
	virtual void OnRtnRepealFromFutureToBankByBank(CThostFtdcRspRepealField* pRspRepeal);

	///�ڻ����������ʽ�ת�ڻ�֪ͨ
	virtual void OnRtnFromBankToFutureByFuture(CThostFtdcRspTransferField* pRspTransfer);

	///�ڻ������ڻ��ʽ�ת����֪ͨ
	virtual void OnRtnFromFutureToBankByFuture(CThostFtdcRspTransferField* pRspTransfer);

	///ϵͳ����ʱ�ڻ����ֹ������������ת�ڻ��������д�����Ϻ��̷��ص�֪ͨ
	virtual void OnRtnRepealFromBankToFutureByFutureManual(CThostFtdcRspRepealField* pRspRepeal);

	///ϵͳ����ʱ�ڻ����ֹ���������ڻ�ת�����������д�����Ϻ��̷��ص�֪ͨ
	virtual void OnRtnRepealFromFutureToBankByFutureManual(CThostFtdcRspRepealField* pRspRepeal);

	///�ڻ������ѯ�������֪ͨ
	virtual void OnRtnQueryBankBalanceByFuture(CThostFtdcNotifyQueryAccountField* pNotifyQueryAccount);

	///�ڻ����������ʽ�ת�ڻ�����ر�
	virtual void OnErrRtnBankToFutureByFuture(CThostFtdcReqTransferField* pReqTransfer, CThostFtdcRspInfoField* pRspInfo);

	///�ڻ������ڻ��ʽ�ת���д���ر�
	virtual void OnErrRtnFutureToBankByFuture(CThostFtdcReqTransferField* pReqTransfer, CThostFtdcRspInfoField* pRspInfo);

	///ϵͳ����ʱ�ڻ����ֹ������������ת�ڻ�����ر�
	virtual void OnErrRtnRepealBankToFutureByFutureManual(CThostFtdcReqRepealField* pReqRepeal, CThostFtdcRspInfoField* pRspInfo);

	///ϵͳ����ʱ�ڻ����ֹ���������ڻ�ת���д���ر�
	virtual void OnErrRtnRepealFutureToBankByFutureManual(CThostFtdcReqRepealField* pReqRepeal, CThostFtdcRspInfoField* pRspInfo);

	///�ڻ������ѯ����������ر�
	virtual void OnErrRtnQueryBankBalanceByFuture(CThostFtdcReqQueryAccountField* pReqQueryAccount, CThostFtdcRspInfoField* pRspInfo);

	///�ڻ������������ת�ڻ��������д�����Ϻ��̷��ص�֪ͨ
	virtual void OnRtnRepealFromBankToFutureByFuture(CThostFtdcRspRepealField* pRspRepeal);

	///�ڻ���������ڻ�ת�����������д�����Ϻ��̷��ص�֪ͨ
	virtual void OnRtnRepealFromFutureToBankByFuture(CThostFtdcRspRepealField* pRspRepeal);

	///�ڻ����������ʽ�ת�ڻ�Ӧ��
	virtual void OnRspFromBankToFutureByFuture(CThostFtdcReqTransferField* pReqTransfer, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�ڻ������ڻ��ʽ�ת����Ӧ��
	virtual void OnRspFromFutureToBankByFuture(CThostFtdcReqTransferField* pReqTransfer, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///�ڻ������ѯ�������Ӧ��
	virtual void OnRspQueryBankAccountMoneyByFuture(CThostFtdcReqQueryAccountField* pReqQueryAccount, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///���з������ڿ���֪ͨ
	virtual void OnRtnOpenAccountByBank(CThostFtdcOpenAccountField* pOpenAccount);

	///���з�����������֪ͨ
	virtual void OnRtnCancelAccountByBank(CThostFtdcCancelAccountField* pCancelAccount);

	///���з����������˺�֪ͨ
	virtual void OnRtnChangeAccountByBank(CThostFtdcChangeAccountField* pChangeAccount);

	void ReqLogin(const char* Investor, const char* PWD);

	void ReqConnect(char* f, const char* b);

	void ReqConnect(int frontID);
	
	int iReqID;
	char* broker, *investor;
	CThostFtdcTraderApi* pUserApi;

public:
	bool m_bFrontConnected;
	bool m_bUserLogin;

private:
	int addrID;
	char* frontAddr;

	bool IsErrorRspInfo(CThostFtdcRspInfoField* pRspInfo)
	{
		// ���ErrorID != 0, ˵���յ��˴������Ӧ
		bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
		if (bResult)
			cerr << "--->>> ErrorID=" << pRspInfo->ErrorID
			<< ", ErrorMsg=" << pRspInfo->ErrorMsg << endl;
		return bResult;
	}

public:
	/** status and message, intertransfer with other application*/
	//��¼ǰ��
	static const int STATUS_FRONT_CONNECTING = 0X00000010;
	static const int STATUS_FRONT_DISCONNECTED = STATUS_FRONT_CONNECTING+1;
	static const int STATUS_FRONT_CONNECTION_FAIL = STATUS_FRONT_CONNECTING+2; // front address

	// �û���¼
	static const int STATUS_USER_LOGINING = 0X00000020;
	static const int STATUS_USER_LOGIN_FAIL = STATUS_USER_LOGINING+1;

	//������Ϣȷ��
	static const int STATUS_SETTLEMENTINFOCONFIRM_QRYING = 0X00000030;
	static const int STATUS_SETTLEMENTINFOCONFIRM_QRY_FAIL = STATUS_SETTLEMENTINFOCONFIRM_QRYING+1;

	//��ѯ����ȷ��
	static const int STATUS_SETTLEMENTINFOCONFIRM_REQING = 0X00000040;
	static const int STATUS_SETTLEMENTINFOCONFIRM_REQ_FAIL = STATUS_SETTLEMENTINFOCONFIRM_REQING+1;
	
	//��ѯ������Ϣ
	static const int STATUS_QRYSETTLEMENTINFO_REQING = 0x00000050;
	static const int STATUS_QRYSETTLEMENTINFO_REQ_FAIL = STATUS_QRYSETTLEMENTINFO_REQING+1;

	//���Լ
	static const int STATUS_INSTRUMENT_QRYING = 0x00000060;
	static const int STATUS_INSTRUMENT_QRY_FAIL = STATUS_INSTRUMENT_QRYING + 1;

	//���Լ
	static const int STATUS_INVESTORPOSITION_QRYING = 0x00000070;
	static const int STATUS_INVESTORPOSITION_QRY_FAIL = STATUS_INVESTORPOSITION_QRYING + 1;
	int status;
	std::string message;
	/*******/

	std::string settleInfo;
	char* tradingDay;
	//ÿ�γ������У��ᴴ��һ����һ�޶���guid����guid�������orderrefһ���������ʹ�õ�orderref����֤orderref���ظ�
	std::string current_guid;
	int orderRef;
	// ��¼��ǰ������orderref��ʼֵ��ͨ����ֵ�ж��Ƿ��ǵ�ǰ���̷����ģ�ÿ���õ�ǰ��ʱ��*10000
	// �÷�����ʱֻ�ܼ��ٳ�ͻ�������ܱ�֤û�г�ͻ
	int orderRefInitValue;
	void tickToFile(CThostFtdcDepthMarketDataField f);
	virtual void pushOrders(int ticks, int seconds, const char* instrument, double price, int director, int offset, int volume);
	virtual void ReqQryInvestorPositionDetail();

private:
	const std::string get_trade_status_json_string();

	// ��ѯ�ֲָ���������ͬʱ�������ֲֲ�ѯ������ÿ��ֻ������һ������Ը��ʧʱ�䣬ҲҪ��֤����׼ȷ
	boost::mutex pendingInstrumentIds_mutex;
	std::vector<std::string> allPendingQryInvestorPositions;
public:
	// �����ڼ䣬�������public
	void reqQryInvestorPositionInQueue(const char *_instrumentID);
	/*bool CanTradeExternal(const char * _instrumentID) {
		if (m_bRequestInProgress
			|| !m_bCompleteLoginProgress) return false;
		std::vector<std::string>::iterator it = std::find(allPendingQryInvestorPositions.begin(), allPendingQryInvestorPositions.end(),_instrumentID);
		return it == allPendingQryInvestorPositions.end();
	}*/
	//std::string pendQryInvestorPositionInstrument;
private:
	int current_ctp_address_index = 0;

	//��¼�ɹ������е���������У�Ȼ��ִ��
	boost::mutex mtx_item_open_close;
	std::map<std::string, std::vector<TradeQuqueItemOpenClose>> allTradeQueueItemOpenCloses;
	boost::mutex mtx_item_query;
	std::map<std::string, std::vector<TradeQuqueItemQueryPosition>> allTradeQueueItemQueryPositions;
	boost::mutex mtx_item_cancel;
	std::map<std::string, std::vector<TradeQuqueItemCancelOrder>> allTradeQueueItemCancelOrders;

	// ��ѯ��ĳ��Ʒ�ֵĳֲֺ�������еĲ�ѯҪ��
	void RemoveQueryPositionTradeQueueItem(const char * instrument) {
		boost::lock_guard<boost::mutex> guard(mtx_item_query);
		if (instrument != nullptr) {
			//������ѯ
			allTradeQueueItemQueryPositions[instrument].erase(allTradeQueueItemQueryPositions[instrument].begin());
			//allTradeQueueItemQueryPositions[instrument].clear();
		}
	}

	// ��ѯ��ĳ��Ʒ�ֵĳֲֺ�������еĿ���Ҫ��
	// ������Ӧ��Ҳ������еĿ���Ҫ��
	void RemoveTradeQueueItem(const char * instrument) {
		if (strlen(instrument) == 0) return;
		
		boost::lock_guard<boost::mutex> guard(mtx_item_open_close);
		allTradeQueueItemOpenCloses[instrument].clear();
	}

	//��Ӧ����������Ҳ����ˣ����������Ҫ��
	//void RemoveTradeQueueItem(const char * instrument,const char* orderref) {
	//	//if (allTradeQueueItemOpenCloses.[instrument].empty())return;
	//	boost::lock_guard<boost::mutex> guard(mtx_item_open_close);
	//	std::vector<TradeQuqueItemOpenClose>::iterator it;
	//	for (it = allTradeQueueItemOpenCloses[instrument].begin(); it != allTradeQueueItemOpenCloses[instrument].end(); ++it) {
	//		if (it->_instrument.compare(instrument) == 0
	//			&& it->_orderref.compare(orderref) ==0)
	//		{
	//			//ɾ������
	//			allTradeQueueItemOpenCloses[instrument].erase(it);
	//			return;
	//		}
	//	}
	//}

	// cancel order
	bool RemoveTradeQueueItem(const char * instrument, int session, int frontid, const char *orderref) {
		boost::lock_guard<boost::mutex> guard(mtx_item_cancel);
		TradeQuqueItemCancelOrder item(instrument, session, frontid, orderref);
		std::vector<TradeQuqueItemCancelOrder>::iterator it = 
			std::remove(allTradeQueueItemCancelOrders[instrument].begin(), allTradeQueueItemCancelOrders[instrument].end(), item);
		if (it == allTradeQueueItemCancelOrders[instrument].end()) return false;

		allTradeQueueItemCancelOrders[instrument].erase(it, allTradeQueueItemCancelOrders[instrument].end());

		return true;
	}

	// order insert
	void RemoveTradeQueueItem(const char * ins, double price, int director, int offset, int volume) {
		boost::lock_guard<boost::mutex> guard(mtx_item_open_close);
		TradeQuqueItemOpenClose item(ins, price, director, offset, volume);
		std::vector<TradeQuqueItemOpenClose>::iterator it =
			std::remove(allTradeQueueItemOpenCloses[ins].begin(), allTradeQueueItemOpenCloses[ins].end(), item);

		allTradeQueueItemOpenCloses[ins].erase(it, allTradeQueueItemOpenCloses[ins].end());
	}

	// set order ref
	void SetTradeQueueItemOrderRef(const char * ins, double price, int director, int offset, int volume,const char * orderref) {
		TradeQuqueItemOpenClose item(ins, price, director, offset, volume);
		
		std::vector<TradeQuqueItemOpenClose>::iterator it =
			std::find(allTradeQueueItemOpenCloses[ins].begin(), allTradeQueueItemOpenCloses[ins].end(), item);
		if (it != allTradeQueueItemOpenCloses[ins].end())
		{
			it->_orderref = orderref;
		}
	}

	void ClearTradeQueueItem() {
		{
			boost::lock_guard<boost::mutex> guard(mtx_item_open_close);
			allTradeQueueItemOpenCloses.clear();
		}
		{
			boost::lock_guard<boost::mutex> guard(mtx_item_cancel);
			allTradeQueueItemCancelOrders.clear();
		}
		{
			boost::lock_guard<boost::mutex> guard(mtx_item_query);
			allTradeQueueItemQueryPositions.clear();
		}
	}
public:
	bool CanTradeExternal() {
		if (!m_bCompleteLoginProgress
			|| m_bRequestInProgress) return false;

		{
			boost::lock_guard<boost::mutex> guard(mtx_item_cancel);
			std::map<std::string, std::vector<TradeQuqueItemCancelOrder>>::iterator it;
			for (it = allTradeQueueItemCancelOrders.begin(); it != allTradeQueueItemCancelOrders.end(); ++it)
			{
				if (it->second.size() > 0) return false;
			}
		}

		{
			boost::lock_guard<boost::mutex> guard(mtx_item_query);
			std::map<std::string, std::vector<TradeQuqueItemQueryPosition>>::iterator it;
			for (it = allTradeQueueItemQueryPositions.begin(); it != allTradeQueueItemQueryPositions.end(); ++it)
			{
				if (it->second.size() > 0) return false;
			}
		}

		{
			boost::lock_guard<boost::mutex> guard(mtx_item_open_close);
			std::map<std::string, std::vector<TradeQuqueItemOpenClose>>::iterator it;
			for (it = allTradeQueueItemOpenCloses.begin(); it != allTradeQueueItemOpenCloses.end(); ++it)
			{
				if (it->second.size() > 0) return false;
			}
		}

		return true;
		/*return allTradeQueueItemOpenCloses.empty()
			&& allTradeQueueItemQueryPositions.empty()
			&& allTradeQueueItemCancelOrders.empty();*/
	}
};

#endif // TRADE_H
