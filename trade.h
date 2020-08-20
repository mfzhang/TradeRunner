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
	TRADEQUEUEITEM_QUERY_POSITION, //查询开仓数量
	TRADEQUEUEITEM_QUERY_ACCOUNT, //查询账户
	TRADEQUEUEITEM_CANCELEORDER//取消挂单
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
	//感觉 ctp是同步处理的，如果之前的事件没有处理完，后面的都不处理
	//所以加一个变量表示当前是否有request正在被处理
	bool m_bRequestInProgress = false;
	//这个表示是否登录流程结束，也就是说包括登录，查询所有的，
	//可以作为是否在线的标注
	bool m_bCompleteLoginProgress = false;
	//交易结束后，为准确计算持仓量，重新查询一遍
	bool m_bQryInverstorPositionAfterTrade = false;
	/* const*/
	static const int MAX_REQUEST_TRY_TIMES = 10;
	/* boost asio*/
	boost::asio::io_service &tradeio;
private:
	int m_nTryTimes = 0;
	//void ResetTryTimes() { 
	//	//只有当没有查询业务的时候才做
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

	/*******查询结算确认************/
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

	/*******查合约************/
	void handle_wait_qryinstrument(const boost::system::error_code& error);
	boost::asio::deadline_timer *qryInstrumentTime;
	void ReqQryInstrument();
	/****************************/

	/*******查持仓************/
	boost::asio::deadline_timer queueQryinvestorpositionTimer;
	void handle_queue_qryinvestorposition(const boost::system::error_code& error);
	void handle_wait_qryinvestorposition(const boost::system::error_code& error, const char* _instrumentId);
	boost::asio::deadline_timer *qryInvestorPositionTime;
	// 标志当前正在查询的合约，有些时候不需要查询所有的合约
	std::string m_currentQryInstrumentId;
	virtual void ReqQryInvestorPosition();
	virtual void ReqQryInvestorPosition(const char *_instrumentID);
	/****************************/

	/*******挂单************/
	void handle_wait_orderinsert(const boost::system::error_code& error, char ordertype,const char* _instrumentId, double price, int director, int offset, int volume);
	boost::asio::deadline_timer *orderinsertTime;
	virtual int ReqOrderInsert(const char* instrument, double price, int director, int offset, int volume);
	virtual int ReqOrderInsert(const char* instrument, int director, int offset, int volume);
	
	/*******************/

	/*******撤单，修改暂时不支持************/
	void handle_wait_orderaction(const boost::system::error_code& error, const char* instrument, int session, int frontid, const char* orderref);
	boost::asio::deadline_timer *orderactionTime;
	virtual void ReqOrderAction(const char* instrument, int session, int frontid, const char* orderref);
	/*******************/

	// other timer
	// 1秒钟更新一次redis，连接状态
	boost::asio::deadline_timer tradeStatusUpdateTimer;
	/*******connect************/
	void handle_wait_status_update(const boost::system::error_code& error);

	ctp_redis_worker &redis_worker;
public:
	//报单和撤单的外部调用
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
		pUserApi = NULL;	//方便程序判断是否连接


		m_bFrontConnected = false;
		m_bUserLogin = false;


		tradingDay = new char[9];
		memset(tradingDay, 0, 9);
		// 记录当前启动的orderref初始值，通过该值判断是否是当前进程发出的，每次用当前的时间+10000
		// 该方案暂时只能减少冲突，并不能保证没有冲突
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
			= new boost::asio::deadline_timer(io);//先都用一个，表示一个时刻只能有一个查询请求
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

	///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
	virtual void OnFrontConnected();

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

	///客户端认证响应
	virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);


	///登录请求响应
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///登出请求响应
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///用户口令更新请求响应
	virtual void OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField* pUserPasswordUpdate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///资金账户口令更新请求响应
	virtual void OnRspTradingAccountPasswordUpdate(CThostFtdcTradingAccountPasswordUpdateField* pTradingAccountPasswordUpdate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///报单录入请求响应
	virtual void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///预埋单录入请求响应
	virtual void OnRspParkedOrderInsert(CThostFtdcParkedOrderField* pParkedOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///预埋撤单录入请求响应
	virtual void OnRspParkedOrderAction(CThostFtdcParkedOrderActionField* pParkedOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///报单操作请求响应
	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///查询最大报单数量响应
	virtual void OnRspQueryMaxOrderVolume(CThostFtdcQueryMaxOrderVolumeField* pQueryMaxOrderVolume, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///投资者结算结果确认响应
	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///删除预埋单响应
	virtual void OnRspRemoveParkedOrder(CThostFtdcRemoveParkedOrderField* pRemoveParkedOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///删除预埋撤单响应
	virtual void OnRspRemoveParkedOrderAction(CThostFtdcRemoveParkedOrderActionField* pRemoveParkedOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询报单响应
	virtual void OnRspQryOrder(CThostFtdcOrderField* pOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询成交响应
	virtual void OnRspQryTrade(CThostFtdcTradeField* pTrade, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询投资者持仓响应
	virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询资金账户响应
	virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField* pTradingAccount, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询投资者响应
	virtual void OnRspQryInvestor(CThostFtdcInvestorField* pInvestor, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询交易编码响应
	virtual void OnRspQryTradingCode(CThostFtdcTradingCodeField* pTradingCode, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询合约保证金率响应
	virtual void OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField* pInstrumentMarginRate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询合约手续费率响应
	virtual void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField* pInstrumentCommissionRate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询交易所响应
	virtual void OnRspQryExchange(CThostFtdcExchangeField* pExchange, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询合约响应
	virtual void OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询行情响应
	virtual void OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询投资者结算结果响应
	virtual void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField* pSettlementInfo, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询转帐银行响应
	virtual void OnRspQryTransferBank(CThostFtdcTransferBankField* pTransferBank, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询投资者持仓明细响应
	virtual void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField* pInvestorPositionDetail, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询客户通知响应
	virtual void OnRspQryNotice(CThostFtdcNoticeField* pNotice, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询结算信息确认响应
	virtual void OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询投资者持仓明细响应
	virtual void OnRspQryInvestorPositionCombineDetail(CThostFtdcInvestorPositionCombineDetailField* pInvestorPositionCombineDetail, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///查询保证金监管系统经纪公司资金账户密钥响应
	virtual void OnRspQryCFMMCTradingAccountKey(CThostFtdcCFMMCTradingAccountKeyField* pCFMMCTradingAccountKey, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询仓单折抵信息响应
	virtual void OnRspQryEWarrantOffset(CThostFtdcEWarrantOffsetField* pEWarrantOffset, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询转帐流水响应
	virtual void OnRspQryTransferSerial(CThostFtdcTransferSerialField* pTransferSerial, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询银期签约关系响应
	//virtual void OnRspQryAccountregister(CThostFtdcAccountregisterField* pAccountregister, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);
	///请求查询银期签约关系响应
	virtual void OnRspQryAccountregister(CThostFtdcAccountregisterField *pAccountregister, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	///错误应答
	virtual void OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///报单通知
	virtual void OnRtnOrder(CThostFtdcOrderField* pOrder);

	///成交通知
	virtual void OnRtnTrade(CThostFtdcTradeField* pTrade);

	///报单录入错误回报
	virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo);

	///报单操作错误回报
	virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField* pOrderAction, CThostFtdcRspInfoField* pRspInfo);

	///合约交易状态通知
	virtual void OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField* pInstrumentStatus);

	///交易通知
	virtual void OnRtnTradingNotice(CThostFtdcTradingNoticeInfoField* pTradingNoticeInfo);

	///提示条件单校验错误
	virtual void OnRtnErrorConditionalOrder(CThostFtdcErrorConditionalOrderField* pErrorConditionalOrder);

	///请求查询签约银行响应
	virtual void OnRspQryContractBank(CThostFtdcContractBankField* pContractBank, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询预埋单响应
	virtual void OnRspQryParkedOrder(CThostFtdcParkedOrderField* pParkedOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询预埋撤单响应
	virtual void OnRspQryParkedOrderAction(CThostFtdcParkedOrderActionField* pParkedOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询交易通知响应
	virtual void OnRspQryTradingNotice(CThostFtdcTradingNoticeField* pTradingNotice, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询经纪公司交易参数响应
	virtual void OnRspQryBrokerTradingParams(CThostFtdcBrokerTradingParamsField* pBrokerTradingParams, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询经纪公司交易算法响应
	virtual void OnRspQryBrokerTradingAlgos(CThostFtdcBrokerTradingAlgosField* pBrokerTradingAlgos, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///银行发起银行资金转期货通知
	virtual void OnRtnFromBankToFutureByBank(CThostFtdcRspTransferField* pRspTransfer);

	///银行发起期货资金转银行通知
	virtual void OnRtnFromFutureToBankByBank(CThostFtdcRspTransferField* pRspTransfer);

	///银行发起冲正银行转期货通知
	virtual void OnRtnRepealFromBankToFutureByBank(CThostFtdcRspRepealField* pRspRepeal);

	///银行发起冲正期货转银行通知
	virtual void OnRtnRepealFromFutureToBankByBank(CThostFtdcRspRepealField* pRspRepeal);

	///期货发起银行资金转期货通知
	virtual void OnRtnFromBankToFutureByFuture(CThostFtdcRspTransferField* pRspTransfer);

	///期货发起期货资金转银行通知
	virtual void OnRtnFromFutureToBankByFuture(CThostFtdcRspTransferField* pRspTransfer);

	///系统运行时期货端手工发起冲正银行转期货请求，银行处理完毕后报盘发回的通知
	virtual void OnRtnRepealFromBankToFutureByFutureManual(CThostFtdcRspRepealField* pRspRepeal);

	///系统运行时期货端手工发起冲正期货转银行请求，银行处理完毕后报盘发回的通知
	virtual void OnRtnRepealFromFutureToBankByFutureManual(CThostFtdcRspRepealField* pRspRepeal);

	///期货发起查询银行余额通知
	virtual void OnRtnQueryBankBalanceByFuture(CThostFtdcNotifyQueryAccountField* pNotifyQueryAccount);

	///期货发起银行资金转期货错误回报
	virtual void OnErrRtnBankToFutureByFuture(CThostFtdcReqTransferField* pReqTransfer, CThostFtdcRspInfoField* pRspInfo);

	///期货发起期货资金转银行错误回报
	virtual void OnErrRtnFutureToBankByFuture(CThostFtdcReqTransferField* pReqTransfer, CThostFtdcRspInfoField* pRspInfo);

	///系统运行时期货端手工发起冲正银行转期货错误回报
	virtual void OnErrRtnRepealBankToFutureByFutureManual(CThostFtdcReqRepealField* pReqRepeal, CThostFtdcRspInfoField* pRspInfo);

	///系统运行时期货端手工发起冲正期货转银行错误回报
	virtual void OnErrRtnRepealFutureToBankByFutureManual(CThostFtdcReqRepealField* pReqRepeal, CThostFtdcRspInfoField* pRspInfo);

	///期货发起查询银行余额错误回报
	virtual void OnErrRtnQueryBankBalanceByFuture(CThostFtdcReqQueryAccountField* pReqQueryAccount, CThostFtdcRspInfoField* pRspInfo);

	///期货发起冲正银行转期货请求，银行处理完毕后报盘发回的通知
	virtual void OnRtnRepealFromBankToFutureByFuture(CThostFtdcRspRepealField* pRspRepeal);

	///期货发起冲正期货转银行请求，银行处理完毕后报盘发回的通知
	virtual void OnRtnRepealFromFutureToBankByFuture(CThostFtdcRspRepealField* pRspRepeal);

	///期货发起银行资金转期货应答
	virtual void OnRspFromBankToFutureByFuture(CThostFtdcReqTransferField* pReqTransfer, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///期货发起期货资金转银行应答
	virtual void OnRspFromFutureToBankByFuture(CThostFtdcReqTransferField* pReqTransfer, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///期货发起查询银行余额应答
	virtual void OnRspQueryBankAccountMoneyByFuture(CThostFtdcReqQueryAccountField* pReqQueryAccount, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///银行发起银期开户通知
	virtual void OnRtnOpenAccountByBank(CThostFtdcOpenAccountField* pOpenAccount);

	///银行发起银期销户通知
	virtual void OnRtnCancelAccountByBank(CThostFtdcCancelAccountField* pCancelAccount);

	///银行发起变更银行账号通知
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
		// 如果ErrorID != 0, 说明收到了错误的响应
		bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
		if (bResult)
			cerr << "--->>> ErrorID=" << pRspInfo->ErrorID
			<< ", ErrorMsg=" << pRspInfo->ErrorMsg << endl;
		return bResult;
	}

public:
	/** status and message, intertransfer with other application*/
	//登录前置
	static const int STATUS_FRONT_CONNECTING = 0X00000010;
	static const int STATUS_FRONT_DISCONNECTED = STATUS_FRONT_CONNECTING+1;
	static const int STATUS_FRONT_CONNECTION_FAIL = STATUS_FRONT_CONNECTING+2; // front address

	// 用户登录
	static const int STATUS_USER_LOGINING = 0X00000020;
	static const int STATUS_USER_LOGIN_FAIL = STATUS_USER_LOGINING+1;

	//结算信息确认
	static const int STATUS_SETTLEMENTINFOCONFIRM_QRYING = 0X00000030;
	static const int STATUS_SETTLEMENTINFOCONFIRM_QRY_FAIL = STATUS_SETTLEMENTINFOCONFIRM_QRYING+1;

	//查询结算确认
	static const int STATUS_SETTLEMENTINFOCONFIRM_REQING = 0X00000040;
	static const int STATUS_SETTLEMENTINFOCONFIRM_REQ_FAIL = STATUS_SETTLEMENTINFOCONFIRM_REQING+1;
	
	//查询结算信息
	static const int STATUS_QRYSETTLEMENTINFO_REQING = 0x00000050;
	static const int STATUS_QRYSETTLEMENTINFO_REQ_FAIL = STATUS_QRYSETTLEMENTINFO_REQING+1;

	//查合约
	static const int STATUS_INSTRUMENT_QRYING = 0x00000060;
	static const int STATUS_INSTRUMENT_QRY_FAIL = STATUS_INSTRUMENT_QRYING + 1;

	//查合约
	static const int STATUS_INVESTORPOSITION_QRYING = 0x00000070;
	static const int STATUS_INVESTORPOSITION_QRY_FAIL = STATUS_INVESTORPOSITION_QRYING + 1;
	int status;
	std::string message;
	/*******/

	std::string settleInfo;
	char* tradingDay;
	//每次程序运行，会创建一个独一无二的guid，该guid和下面的orderref一起组成最终使用的orderref，保证orderref不重复
	std::string current_guid;
	int orderRef;
	// 记录当前启动的orderref初始值，通过该值判断是否是当前进程发出的，每次用当前的时间*10000
	// 该方案暂时只能减少冲突，并不能保证没有冲突
	int orderRefInitValue;
	void tickToFile(CThostFtdcDepthMarketDataField f);
	virtual void pushOrders(int ticks, int seconds, const char* instrument, double price, int director, int offset, int volume);
	virtual void ReqQryInvestorPositionDetail();

private:
	const std::string get_trade_status_json_string();

	// 查询持仓辅助，可能同时发起多个持仓查询，但是每次只能运行一个，宁愿损失时间，也要保证数据准确
	boost::mutex pendingInstrumentIds_mutex;
	std::vector<std::string> allPendingQryInvestorPositions;
public:
	// 测试期间，这个方法public
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

	//登录成功后所有的任务都入队列，然后执行
	boost::mutex mtx_item_open_close;
	std::map<std::string, std::vector<TradeQuqueItemOpenClose>> allTradeQueueItemOpenCloses;
	boost::mutex mtx_item_query;
	std::map<std::string, std::vector<TradeQuqueItemQueryPosition>> allTradeQueueItemQueryPositions;
	boost::mutex mtx_item_cancel;
	std::map<std::string, std::vector<TradeQuqueItemCancelOrder>> allTradeQueueItemCancelOrders;

	// 查询完某个品种的持仓后，清掉所有的查询要求
	void RemoveQueryPositionTradeQueueItem(const char * instrument) {
		boost::lock_guard<boost::mutex> guard(mtx_item_query);
		if (instrument != nullptr) {
			//继续查询
			allTradeQueueItemQueryPositions[instrument].erase(allTradeQueueItemQueryPositions[instrument].begin());
			//allTradeQueueItemQueryPositions[instrument].clear();
		}
	}

	// 查询完某个品种的持仓后，清掉所有的开仓要求
	// 报单响应后，也清空所有的开仓要求
	void RemoveTradeQueueItem(const char * instrument) {
		if (strlen(instrument) == 0) return;
		
		boost::lock_guard<boost::mutex> guard(mtx_item_open_close);
		allTradeQueueItemOpenCloses[instrument].clear();
	}

	//相应的情况好像找不到了，这个好像不需要了
	//void RemoveTradeQueueItem(const char * instrument,const char* orderref) {
	//	//if (allTradeQueueItemOpenCloses.[instrument].empty())return;
	//	boost::lock_guard<boost::mutex> guard(mtx_item_open_close);
	//	std::vector<TradeQuqueItemOpenClose>::iterator it;
	//	for (it = allTradeQueueItemOpenCloses[instrument].begin(); it != allTradeQueueItemOpenCloses[instrument].end(); ++it) {
	//		if (it->_instrument.compare(instrument) == 0
	//			&& it->_orderref.compare(orderref) ==0)
	//		{
	//			//删除返回
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
