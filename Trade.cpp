#include "stdafx.h"
#define ELPP_THREAD_SAFE  
//
#include "trade.h"
//#include "quote.h"
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
#include "my_utils.h"
#include "ctp_message_def.h"
#include <boost/thread/lock_guard.hpp>

void Trade::ReqConnect(char* f, const char* b)
{
	if (pUserApi == NULL)
	{
		pUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi("logTrade");
	}
	else {
		//pUserApi->Release();
		pUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi("logTrade");
	}
	pUserApi->RegisterSpi((CThostFtdcTraderSpi*)this);
	pUserApi->SubscribePublicTopic(THOST_TERT_RESTART);
	pUserApi->SubscribePrivateTopic(THOST_TERT_RESTART);

	//this->addrID = frontID;

	strcpy(this->broker, b);

	pUserApi->RegisterFront(f);
	pUserApi->Init();

	status = STATUS_FRONT_CONNECTING;
	message = "Connecting front.....";
	
	// ���͵�redis
	redis_worker.update_ctp_account_login_info(TradeContext::get_mutable_instance().argus.get_username().c_str(), get_trade_status_json_string().c_str());
	//delete addr1;
}

void Trade::ReqConnect(int frontID)
{
	if (pUserApi == NULL)
	{
		pUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi("logTrade");
	}
	else {
		pUserApi->Release();
		pUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi("logTrade");
	}
	pUserApi->RegisterSpi((CThostFtdcTraderSpi*)this);
	pUserApi->SubscribePublicTopic(THOST_TERT_RESTART);
	pUserApi->SubscribePrivateTopic(THOST_TERT_RESTART);

	this->addrID = frontID;
	char* addr1 = new char[256];

	//ģ��
	if (frontID == 0)
	{
		strcpy(this->broker, "2030");
		strcpy(addr1, "tcp://asp-sim2-front1.financial-trading-platform.com:26205");
	}
	else
	{
		strcpy(this->broker, "66666");
		sprintf(addr1, "tcp://ctp1-front%d.citicsf.com:41205", addrID);
	}

	pUserApi->RegisterFront(addr1);
	pUserApi->Init();

	delete addr1;

	status = STATUS_FRONT_CONNECTING;
	message = "Connecting front.....";

	// ���͵�redis
	redis_worker.update_ctp_account_login_info(TradeContext::get_mutable_instance().argus.get_username().c_str(), get_trade_status_json_string().c_str());
}

void Trade::Connect()
{
	LOG(INFO) << "ReqConnecting, try times = "<< m_nTryTimes;
	ReqConnect((char*)TradeContext::get_mutable_instance().argus.get_addresses()[current_ctp_address_index].c_str(),
		TradeContext::get_mutable_instance().argus.get_broker().c_str());
	frontConnectTime->expires_from_now(boost::posix_time::seconds(10));
	frontConnectTime->async_wait(boost::bind(&Trade::handle_wait_connect, this, boost::asio::placeholders::error));
}

void Trade::Login()
{
	
	ReqLogin(TradeContext::get_mutable_instance().argus.get_username().c_str()
		, TradeContext::get_mutable_instance().argus.get_password().c_str());

	status = STATUS_USER_LOGINING;
	message = boost::str(boost::format("ReqLogining, try times = %1%") %m_nTryTimes);
	redis_worker.update_ctp_account_login_info(investor, get_trade_status_json_string().c_str());
	LOG(INFO) << message;

	loginTime->expires_from_now(boost::posix_time::seconds(5));
	loginTime->async_wait(boost::bind(&Trade::handle_wait_login, this, boost::asio::placeholders::error));
}

//����
void Trade::OnFrontConnected()
{
	// ��Ҫ���趨�Ƿ����ӣ���Ϊtimeout��������Ҫ
	m_bFrontConnected = true;
	m_bCompleteLoginProgress = false;
	m_bRequestInProgress = false;

	// ���͵�redis
	message = "OnFrontConnected";
	redis_worker.update_ctp_account_login_info(TradeContext::get_mutable_instance().argus.get_username().c_str(), get_trade_status_json_string().c_str());
	allTradeQueueItemOpenCloses.clear();
	allTradeQueueItemQueryPositions.clear();
	allTradeQueueItemCancelOrders.clear();
	frontConnectTime->cancel();
	//ResetTryTimes();
	TradeContext::get_mutable_instance().reset();
	Login();
	
}

void Trade::handle_wait_login(const boost::system::error_code& error)
{
	if (m_bUserLogin) return;

	//if (m_nTryTimes > MAX_REQUEST_TRY_TIMES)
	{
		status = STATUS_USER_LOGIN_FAIL;
		message = "User Login failed after 5 try times......";
		redis_worker.update_ctp_account_login_info(investor, get_trade_status_json_string().c_str());
		loginTime->get_io_service().stop();
		return;
	}

	/*IncreaseTryTimes();

	Login();*/
}

void Trade::handle_wait_connect(const boost::system::error_code& error)
{
	if (error) return; // ����0��ʾcancel��������995
	
	if (m_bFrontConnected) return;

    //������ӳ�ʱ��������ַ��������ȡ�����еĵ�ַ����һ��
    if (current_ctp_address_index >= TradeContext::get_mutable_instance().argus.get_addresses().size()-1) {
        frontConnectTime->get_io_service().stop();
        return;
    }
    else {
        ++current_ctp_address_index;
        LOG(ERROR) <<"connecting next address:" << TradeContext::get_mutable_instance().argus.get_addresses()[current_ctp_address_index];
        Connect();
    }
    
	//connect failed
	
	//if (m_nTryTimes > MAX_REQUEST_TRY_TIMES)
	//{
	//	// move to next ctp address
	//	if (current_ctp_address_index < TradeContext::get_mutable_instance().argus.get_addresses().size() - 1)
	//	{
	//		++current_ctp_address_index;
	//		ResetTryTimes();//����

	//		LOG(ERROR) << MAX_REQUEST_TRY_TIMES << " times of front connect reached, connecting next address:"<< TradeContext::get_mutable_instance().argus.get_addresses()[current_ctp_address_index];
	//	}
	//	else
	//	{
	//		LOG(ERROR) << MAX_REQUEST_TRY_TIMES << " times of front connect reached";
	//		frontConnectTime->get_io_service().stop();
	//		return;
	//	}
	//}
	//LOG(WARNING) << "Connect failed, trying times:"<< m_nTryTimes;
	//IncreaseTryTimes();

	//Connect();
}


void Trade::Run()
{
	Connect();
}
//�Ͽ�
void Trade::OnFrontDisconnected(int nReason)
{
	m_bFrontConnected = false;
	m_bCompleteLoginProgress = false;
	LOG(WARNING) << "���׶Ͽ�! "<< nReason;

	TradeContext::get_mutable_instance().reset();
    //��������һ�Σ�������Ӳ��Ͼ��˳���
    m_nTryTimes = 0;
    current_ctp_address_index = 0;
	Connect();
}

//����
void Trade::OnHeartBeatWarning(int nTimeLapse)
{
}

//��֤
void Trade::OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

//��¼
void Trade::ReqLogin(const char* Investor, const char* PWD)
{
	strcpy(investor, Investor);

	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, broker);
	strcpy(req.UserID, investor);
	strcpy(req.Password, PWD);

	// һ���ڳ���50��
	int counter = 0;
	int nret = pUserApi->ReqUserLogin(&req, ++iReqID);
	while (nret != 0 && (counter++)<50)
	{
		boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(20));
		nret = pUserApi->ReqUserLogin(&req, ++iReqID);
	}

}
void Trade::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin,
	CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (IsErrorRspInfo(pRspInfo))
	{
		m_bUserLogin = false;
		LOG(ERROR)<<str(boost::format("��¼����(%1%):%2%") % pRspInfo->ErrorID % pRspInfo->ErrorMsg);
        if (pRspInfo->ErrorID == 3) {//���Ϸ��ĵ�¼��������󣬲��ٳ���
            loginTime->get_io_service().stop();
            return;
        }
		loginTime->cancel();//���Ե�½
		Login();
	}
	else
	{
		m_bUserLogin = true;
		loginTime->cancel();// cancel ��ʱ
		//ResetTryTimes();
		ReqQrySettlementInfoConfirm();
	}
}

/*******��ѯ����ȷ��************/
void Trade::handle_wait_qry_settlementinfo_confirm(const boost::system::error_code& error)
{
	if (error) return; // cancel

	//if (m_nTryTimes > MAX_REQUEST_TRY_TIMES)
	{
		status = STATUS_SETTLEMENTINFOCONFIRM_QRY_FAIL;
		message = "��ѯ����ȷ��ʧ��!";
		redis_worker.update_ctp_account_login_info(investor, get_trade_status_json_string().c_str());
		qrysettlementInfoConfirmTime->get_io_service().stop();
		return;
	}

	/*IncreaseTryTimes();

	ReqQrySettlementInfoConfirm();*/
}

void Trade::ReqQrySettlementInfoConfirm()
{
	
	CThostFtdcQrySettlementInfoConfirmField f;
	memset(&f, 0, sizeof(f));
	strcpy(f.BrokerID, broker);
	strcpy(f.InvestorID, TradeContext::get_mutable_instance().argus.get_username().c_str());

	//pUserApi->ReqQrySettlementInfoConfirm(&f, ++iReqID);
	// һ���ڳ���50��
	int counter = 0;
	int nret = pUserApi->ReqQrySettlementInfoConfirm(&f, ++iReqID);
	while (nret != 0 && (counter++)<50)
	{
		boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(20));
		nret = pUserApi->ReqQrySettlementInfoConfirm(&f, ++iReqID);
	}
	status = STATUS_SETTLEMENTINFOCONFIRM_QRYING;
	message = boost::str(boost::format("��ѯ����ȷ��... try time %1%") % m_nTryTimes);
	redis_worker.update_ctp_account_login_info(investor, get_trade_status_json_string().c_str());
	LOG(INFO) <<message;

	qrysettlementInfoConfirmTime->expires_from_now(boost::posix_time::seconds(20));
	qrysettlementInfoConfirmTime->async_wait(boost::bind(&Trade::handle_wait_qry_settlementinfo_confirm, this, boost::asio::placeholders::error));
}

void Trade::OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm,
	CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	qrysettlementInfoConfirmTime->cancel();
	if (this->IsErrorRspInfo(pRspInfo))
	{
		boost::system::error_code error(0, boost::system::system_category());
		handle_wait_qry_settlementinfo_confirm(error);
	}
	else
	{
		strcpy(tradingDay, pUserApi->GetTradingDay());

		if ((pSettlementInfoConfirm) && strcmp(pSettlementInfoConfirm->ConfirmDate, tradingDay) == 0)
		{
			ReqSettlementInfoConfirm();
		}
		else
		{
			// 
			//ResetTryTimes();
			ReqQrySettlementInfo();
		}
	}
}
/********************��ѯ����ȷ�� block ************************************/

/********************ȷ�Ͻ���************************************/
void Trade::ReqSettlementInfoConfirm()
{
	
	CThostFtdcSettlementInfoConfirmField f;
	memset(&f, 0, sizeof(f));
	strcpy(f.BrokerID, broker);
	strcpy(f.InvestorID, TradeContext::get_mutable_instance().argus.get_username().c_str());

	// һ���ڳ���50��
	int counter = 0;
	int nret = pUserApi->ReqSettlementInfoConfirm(&f, ++iReqID);
	while (nret != 0 && (counter++)<50)
	{
		boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(20));
		nret = pUserApi->ReqSettlementInfoConfirm(&f, ++iReqID);
	}

	status = STATUS_SETTLEMENTINFOCONFIRM_REQING;
	message = boost::str(boost::format("ȷ�Ͻ���... try time %1%") % m_nTryTimes);
	redis_worker.update_ctp_account_login_info(investor, get_trade_status_json_string().c_str());
	LOG(INFO) <<message;
	settlementInfoConfirmTime->expires_from_now(boost::posix_time::seconds(120));
	settlementInfoConfirmTime->async_wait(boost::bind(&Trade::handle_wait_settlementinfo_confirm, this, boost::asio::placeholders::error));
}

void Trade::handle_wait_settlementinfo_confirm(const boost::system::error_code& error)
{
	if (error) return; // cancel

	//if (m_nTryTimes > MAX_REQUEST_TRY_TIMES)
	{
		settlementInfoConfirmTime->get_io_service().stop();
		return;
	}

	/*IncreaseTryTimes();

	ReqSettlementInfoConfirm();*/
}

//ȷ�Ͻ���
void Trade::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm,
	CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (bIsLast)
	{
		settlementInfoConfirmTime->cancel();
		//ResetTryTimes();
		ReqQryInstrument();
	}
}


/********************ȷ�Ͻ��� block ************************************/

/********************��ѯ����************************************/
void Trade::ReqQrySettlementInfo()
{
	CThostFtdcQrySettlementInfoField f;
	memset(&f, 0, sizeof(f));
	strcpy(f.BrokerID, broker);
	strcpy(f.InvestorID, TradeContext::get_mutable_instance().argus.get_username().c_str());
	
	// һ���ڳ���50��
	int counter = 0;
	int nret = pUserApi->ReqQrySettlementInfo(&f, ++iReqID);
	while (nret != 0 && (counter++)<50)
	{
		boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(20));
		nret = pUserApi->ReqQrySettlementInfo(&f, ++iReqID);
	}

	status = STATUS_QRYSETTLEMENTINFO_REQING;
	message = boost::str(boost::format("�������Ϣ... try times %1%") % m_nTryTimes);
	redis_worker.update_ctp_account_login_info(investor, get_trade_status_json_string().c_str());
	LOG(INFO) << message;

	qrySettlementInfoTime->expires_from_now(boost::posix_time::seconds(3));
	qrySettlementInfoTime->async_wait(boost::bind(&Trade::handle_wait_qrysettlementinfo, this, boost::asio::placeholders::error));

}


void Trade::handle_wait_qrysettlementinfo(const boost::system::error_code& error)
{
	if (error) return; // cancel

	//if (m_nTryTimes > MAX_REQUEST_TRY_TIMES)
	{
		qrySettlementInfoTime->get_io_service().stop();
		return;
	}

	/*IncreaseTryTimes();
	ReqQrySettlementInfo();*/
}

//�������Ϣ,����
void Trade::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField* pSettlementInfo,
	CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (pSettlementInfo)
		this->settleInfo += pSettlementInfo->Content;

	if (bIsLast)
	{
		qrySettlementInfoTime->cancel();
		//ResetTryTimes();
		ReqSettlementInfoConfirm();
	}
}
/********************��ѯ���� block************************************/


/********************���Լ************************************/
void Trade::handle_wait_qryinstrument(const boost::system::error_code& error)
{
	if (error) return;// cancel
	//if (m_nTryTimes > MAX_REQUEST_TRY_TIMES)
	{
		qryInstrumentTime->get_io_service().stop();
		return;
	}

	/*IncreaseTryTimes();
	ReqQryInstrument();*/

}
void Trade::ReqQryInstrument()
{
	CThostFtdcQryInstrumentField f;
	memset(&f, 0, sizeof(f));
	
	// һ���ڳ���50��
	int counter = 0;
	int nret = pUserApi->ReqQryInstrument(&f, ++iReqID);
	while (nret != 0 && (counter++)<50)
	{
		boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(20));
		nret = pUserApi->ReqQryInstrument(&f, ++iReqID);
	}

	status = STATUS_INSTRUMENT_QRYING;
	message = boost::str(boost::format("���Լ...try times %1%") % m_nTryTimes);
	redis_worker.update_ctp_account_login_info(investor, get_trade_status_json_string().c_str());
	LOG(INFO) << message;
	
	qryInstrumentTime->expires_from_now(boost::posix_time::seconds(20));
	qryInstrumentTime->async_wait(boost::bind(&Trade::handle_wait_qryinstrument, this, boost::asio::placeholders::error));
}

//���Լ
void Trade::OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument,
	CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo))
	{
		if (pInstrument != nullptr && strlen(pInstrument->InstrumentID) > 0)
		{
			TradeContext::get_mutable_instance().m_allInstrumentRuntimes[pInstrument->InstrumentID].instrumentID = pInstrument->InstrumentID;
			TradeContext::get_mutable_instance().m_allInstrumentRuntimes[pInstrument->InstrumentID]._instrument = *pInstrument;
		}
	}
	if (bIsLast)
	{
		qryInstrumentTime->cancel();
		this->redis_worker.update_ctp_instrument_ids();
		
		//ResetTryTimes();
		this->ReqQryInvestorPosition();
	}
}

/********************���Լ block************************************/

/********************��ֲ�************************************/
void Trade::handle_queue_qryinvestorposition(const boost::system::error_code& error) {
	//������Σ���Ҫ������һ��
	queueQryinvestorpositionTimer.expires_from_now(boost::posix_time::microseconds(100));
	queueQryinvestorpositionTimer.async_wait(boost::bind(&Trade::handle_queue_qryinvestorposition, this, boost::asio::placeholders::error));

	// ������������ִ�л����еĶ�������ƽ�֣�ȡ���ҵ������Լ�ȵ�
	// ���������ȼ��ģ���ִ��ȡ���ҵ����ٲ��λ�����ƽ��
	if (m_bRequestInProgress) return;

	{
		std::string instrument;
		int session = 0;
		int frontid = 0;
		std::string orderref;
		{
			boost::lock_guard<boost::mutex> guard(mtx_item_cancel);
			std::map<std::string, std::vector<TradeQuqueItemCancelOrder>>::iterator it;
			for (it = allTradeQueueItemCancelOrders.begin(); it != allTradeQueueItemCancelOrders.end(); ++it) {
				if (it->second.empty()) continue;

				std::vector<TradeQuqueItemCancelOrder>::iterator it1;
				for (it1 = it->second.begin(); it1 != it->second.end(); ++it1) {
					//ResetTryTimes();
					instrument = it1->_instrument;
					session = it1->_session;
					frontid = it1->_frontid;
					orderref = it1->_orderref;
					break;
				}

				if (!instrument.empty()) break;
			}
		}
		if (!instrument.empty())
		{
			ReqOrderAction(instrument.c_str(), session, frontid, orderref.c_str());//�ҵ���һ������
			return;
		}
	}
	{
		std::string instrument;
		bool bfind = false;
		{
			boost::lock_guard<boost::mutex> guard(mtx_item_query);
			std::map<std::string, std::vector<TradeQuqueItemQueryPosition>>::iterator itq;
			for (itq = allTradeQueueItemQueryPositions.begin(); itq != allTradeQueueItemQueryPositions.end(); ++itq) {
				if (itq->second.empty()) continue;

				std::vector<TradeQuqueItemQueryPosition>::iterator it1;
				for (it1 = itq->second.begin(); it1 != itq->second.end(); ++it1) {
					//ResetTryTimes();
					if (it1->_instrument.empty() ||
						it1->_instrument.compare("ALL") == 0)
					{
						//ReqQryInvestorPosition();
						instrument = "";
					}
					else {
						//ReqQryInvestorPosition(it1->_instrument.c_str());
						instrument = it1->_instrument;

					}
					bfind = true;
					break;
				}

				if (bfind)break;
			}
		}
		if (bfind) {
			if (instrument.empty())
			{
				TradeContext::get_mutable_instance().ClearInstrumentInvestorPosition(instrument.c_str());
				ReqQryInvestorPosition();
			}
			else
			{
				//TradeContext::get_mutable_instance().ClearInstrumentInvestorPosition(instrument.c_str());
				ReqQryInvestorPosition(instrument.c_str());
			}

			return;
		}
	}
	{
		std::string instrument;
		int director = 0;
		int offset = 0;
		int volume = 0;
		double price = -1;
		{
			boost::lock_guard<boost::mutex> guard(mtx_item_open_close);
			std::map<std::string, std::vector<TradeQuqueItemOpenClose>>::iterator ito;
			for (ito = allTradeQueueItemOpenCloses.begin(); ito != allTradeQueueItemOpenCloses.end(); ++ito) {
				if (ito->second.empty()) continue;

				std::vector<TradeQuqueItemOpenClose>::iterator it1;
				for (it1 = ito->second.begin(); it1 != ito->second.end(); ++it1) {
					//ResetTryTimes();
					//if (it1->_price < 0) {
					//	//ReqOrderInsert(it1->_instrument.c_str(), it1->_director, it1->_offset, it1->_volume);
					//	instrument = it1->_instrument;
					//	director = it1->_director;
					//	offset = it1->_offset;
					//	volume = it1->_volume;
					//}
					//else {
					//	//ReqOrderInsert(it1->_instrument.c_str(), it1->_price, it1->_director, it1->_offset, it1->_volume);
					//}

					instrument = it1->_instrument;
					director = it1->_director;
					offset = it1->_offset;
					volume = it1->_volume;

					price = it1->_price;
					break;
				}

				if (!instrument.empty())break;
			}
		}
		if (!instrument.empty())
		{
			//С��10�����м۱���
			if(price<10.0)
				ReqOrderInsert(instrument.c_str(), director,offset, volume);
			else
				ReqOrderInsert(instrument.c_str(), price, director, offset, volume);
		}
	}
	return;

	

	//if (m_bRequestInProgress) {
	//	return;
	//}

	//
	//std::string next_instrument_id = "";
	//{
	//	boost::lock_guard<boost::mutex> guard(pendingInstrumentIds_mutex);
	//	if (allPendingQryInvestorPositions.empty()) {
	//		return;
	//	}
	//	m_currentQryInstrumentId = "";
	//	// û�����ڲ�ѯ�ģ�ֱ�Ӳ�ѯ
	//	if (!allPendingQryInvestorPositions.empty())
	//	{
	//		next_instrument_id = allPendingQryInvestorPositions[0];
	//		allPendingQryInvestorPositions.erase(allPendingQryInvestorPositions.begin());
	//		//LOG(DEBUG) << "OnRspQryInvestorPosition: allPendingQryInvestorPositions not empty and query next one," << next_instrument_id;
	//		LOG(DEBUG) << "OnRspQryInvestorPosition: allPendingQryInvestorPositions not empty and query next one,";
	//	}
	//}

	////if(!next_instrument_id.empty())
	//{
	//
	//	LOG(DEBUG) << "reqQryInvestorPositionInQueue(ReqQryInvestorPosition):" << next_instrument_id <<":Start";
	//	//ResetTryTimes();
	//	if (next_instrument_id.compare("ALL") == 0
	//		|| next_instrument_id.length()==0) {
	//		ReqQryInvestorPosition();
	//	}
	//	else {
	//		// ���渳ֵ
	//		ReqQryInvestorPosition(next_instrument_id.c_str());
	//	}
	//	return;
	//}
	
}
void Trade::handle_wait_qryinvestorposition(const boost::system::error_code& error,
											const char* _instrumentId)
{
	if (error)
	{
		return;// cancel
	}

	if (!m_bCompleteLoginProgress)
	{
		qryInvestorPositionTime->get_io_service().stop();
	}
	//��������ѯ
	m_bRequestInProgress = false;

	return;
	if (error)
	{
		return;// cancel
	}
	if (m_nTryTimes > MAX_REQUEST_TRY_TIMES)
	{
		if (!m_bCompleteLoginProgress)
		{
			qryInvestorPositionTime->get_io_service().stop();
		}
		// �ٴη������
		//reqQryInvestorPositionInQueue(_instrumentId);
		m_bRequestInProgress = false;
		return;
	}

	//IncreaseTryTimes();
	if (_instrumentId == nullptr
		||strlen(_instrumentId)==0)
	{
		ReqQryInvestorPosition();
	}
	else
	{
		ReqQryInvestorPosition(_instrumentId);
		//reqQryInvestorPositionInQueue(_instrumentId);
	}


}
void Trade::ReqQryInvestorPosition()
{
	/*if (!m_bCompleteLoginProgress) {
		ClearTradeQueueItem();
		return;
	}
*/
	if (m_bRequestInProgress) {
		return;
	}

	m_bRequestInProgress = true;
	
	m_currentQryInstrumentId = "ALL";
	CThostFtdcQryInvestorPositionField f;
	memset(&f, 0, sizeof(f));
	strcpy(f.BrokerID, broker);
	strcpy(f.InvestorID, investor);
	
	// һ���ڳ���50��
	int counter = 0;
	int nret = pUserApi->ReqQryInvestorPosition(&f, ++iReqID);
	while (nret != 0 && (counter++)<50 && !m_bCompleteLoginProgress)
	{
		boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(20));
		nret = pUserApi->ReqQryInvestorPosition(&f, ++iReqID);
	}

	
	if (nret == 0) {
		status = STATUS_INVESTORPOSITION_QRYING;
		message = boost::str(boost::format("��ֲ�...try times %1%") % m_nTryTimes);
		redis_worker.update_ctp_account_login_info(investor, get_trade_status_json_string().c_str());
		LOG(INFO) << message;
		qryInvestorPositionTime->expires_from_now(boost::posix_time::seconds(3));
		qryInvestorPositionTime->async_wait(boost::bind(&Trade::handle_wait_qryinvestorposition, this, boost::asio::placeholders::error, nullptr));
	}
	else {
		m_bRequestInProgress = false;
	}
}

/*��ֲ�*/
void Trade::ReqQryInvestorPosition(const char* _instrumentID)
{
	if (!m_bCompleteLoginProgress) {
		ClearTradeQueueItem();
		return;
	}

	if (m_bRequestInProgress) {
		return;
	}

	m_bRequestInProgress = true;
	m_currentQryInstrumentId = _instrumentID;

	TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_instrumentID].ClearCThostFtdcInvestorPositionField();

	CThostFtdcQryInvestorPositionField f;
	memset(&f, 0, sizeof(f));
	strncpy(f.InstrumentID, _instrumentID, 30);
	strcpy(f.BrokerID, broker);
	strcpy(f.InvestorID, investor);

	
	// һ���ڳ���50��
	int counter = 0;
	int nret = pUserApi->ReqQryInvestorPosition(&f, ++iReqID);

	//ֻ��ѯһ��
	//while (nret != 0 && (counter++)<50)
	/*{
		boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(20));
		nret = pUserApi->ReqQryInvestorPosition(&f, ++iReqID);
	}*/
	if (nret == 0) {
		status = STATUS_INVESTORPOSITION_QRYING;
		message = boost::str(boost::format("��ֲ�(%1%)...try times %2%") % _instrumentID % m_nTryTimes);
		redis_worker.update_ctp_account_login_info(investor, get_trade_status_json_string().c_str());
		LOG(INFO) << message;

		qryInvestorPositionTime->expires_from_now(boost::posix_time::seconds(3));
		qryInvestorPositionTime->async_wait(boost::bind(&Trade::handle_wait_qryinvestorposition, this, boost::asio::placeholders::error, _instrumentID));
	}
	else {
		//����ѭ����ѯ
		m_bRequestInProgress = false;
	}
	
}
//��ֲ���Ӧ
void Trade::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition,
	CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	//InstrumentID Ϊ��Ʒ����
	//direction: 3 �գ�2��
	if (pInvestorPosition && !IsErrorRspInfo(pRspInfo))
	{
		TradeContext::get_mutable_instance().InsertCThostFtdcInvestorPositionField(pInvestorPosition);
	}

	if (bIsLast)
	{
		//������������ˣ�ʲô���������ˣ�ʣ�µĵ����������
		// �����ѯ���еģ���������
		if (m_currentQryInstrumentId.compare("ALL") == 0)
		{
			redis_worker.update_ctp_CThostFtdcInvestorPositionField(investor);
		}
		else// ����ֻ����ĳ����Լ��
		{
			LOG(DEBUG) << "OnRspQryInvestorPosition:" << m_currentQryInstrumentId << ":Done";
			redis_worker.update_ctp_CThostFtdcInvestorPositionField(investor, m_currentQryInstrumentId.c_str(), TradeContext::get_mutable_instance().m_allInstrumentRuntimes[m_currentQryInstrumentId].investorPositionAllDirections);
		}



		// ���ûص�֪ͨ����
		if (!investorPostionQueryCallback.empty())
		{
			investorPostionQueryCallback();
		}

		if (!m_bCompleteLoginProgress)
		{
			m_bCompleteLoginProgress = true;
			static bool firstrunning = true;
			if (firstrunning)
			{
				firstrunning = false;
				// update to middle ware
				tradeStatusUpdateTimer.expires_from_now(boost::posix_time::seconds(1));
				tradeStatusUpdateTimer.async_wait(boost::bind(&Trade::handle_wait_status_update, this, boost::asio::placeholders::error));

				queueQryinvestorpositionTimer.expires_from_now(boost::posix_time::milliseconds(100));
				queueQryinvestorpositionTimer.async_wait(boost::bind(&Trade::handle_queue_qryinvestorposition, this, boost::asio::placeholders::error));
			}
			// upadate login info
			redis_worker.update_ctp_account_login_info(TradeContext::get_mutable_instance().argus.get_username().c_str(), get_trade_status_json_string().c_str());
		}
		else {
			RemoveQueryPositionTradeQueueItem(m_currentQryInstrumentId.c_str());
			RemoveTradeQueueItem(m_currentQryInstrumentId.c_str());
			qryInvestorPositionTime->cancel();
			m_bRequestInProgress = false;
		}
	}
}

/********************��ֲ� block************************************/
/********************����************************************/
bool Trade::ReqOrderInsertExternal(const char* instrument, double price, int director, int offset, int volume)
{
	if (!m_bCompleteLoginProgress) return false;
	// ����в�ѯ����ͳ�������,������������
	// ����������Ӧ���Լ����ģ�����û�б��汨�����ݣ���ʱ�ͷ������
	// �����ǰ�в�ѯ��ִ�У�Ҳ��������
	if (m_bRequestInProgress)return false;
	{
		boost::lock_guard<boost::mutex> guard(mtx_item_open_close);
		// ����в�ѯ����ͳ������󣬲�����
		TradeQuqueItemQueryPosition posi(instrument);
		std::vector<TradeQuqueItemQueryPosition>::iterator it = std::find(allTradeQueueItemQueryPositions[instrument].begin(), allTradeQueueItemQueryPositions[instrument].end(), posi);
		if (it == allTradeQueueItemQueryPositions[instrument].end()
			&& allTradeQueueItemCancelOrders[instrument].empty()
			&& allTradeQueueItemOpenCloses[instrument].empty()) {

			TradeQuqueItemOpenClose item(instrument, price, director, offset, volume);
			allTradeQueueItemOpenCloses[instrument].push_back(item);
			return true;
		}
		//��ʾû���룬��ǰ�в�ѯ��
		return false;
	}
	/*ResetTryTimes();
	return ReqOrderInsert(instrument,price,director,offset,volume);*/
}
bool Trade::ReqOrderInsertExternal(const char* instrument, int director, int offset, int volume)
{
	if (!m_bCompleteLoginProgress) return false;
	// ����в�ѯ����ͳ�������,������������
	// ����������Ӧ���Լ����ģ�����û�б��汨�����ݣ���ʱ�ͷ������
	// �����ǰ�в�ѯ��ִ�У�Ҳ��������
	if (m_bRequestInProgress)return false;
	{
		boost::lock_guard<boost::mutex> guard(mtx_item_open_close);
		TradeQuqueItemQueryPosition posi(instrument);
		std::vector<TradeQuqueItemQueryPosition>::iterator it = std::find(allTradeQueueItemQueryPositions[instrument].begin(), allTradeQueueItemQueryPositions[instrument].end(), posi);
		if (it == allTradeQueueItemQueryPositions[instrument].end()
			&& allTradeQueueItemCancelOrders[instrument].empty()
			&& allTradeQueueItemOpenCloses[instrument].empty()) {
			TradeQuqueItemOpenClose item(instrument, -1, director, offset, volume);
			allTradeQueueItemOpenCloses[instrument].push_back(item);
			return true;
		}

		//��ʾû���룬��ǰ�в�ѯ�������߹ҵ�
		return false;
	}
	/*ResetTryTimes();
	return ReqOrderInsert(instrument,director,offset,volume);*/
}
bool Trade::ReqOrderActionExternal(const char* instrument, int session, int frontid, const char* orderref)
{
	if (instrument == nullptr
		|| strlen(instrument) == 0) return false;
	// �����ǰ�в�ѯ��ִ�У�Ҳ��������
	if (m_bRequestInProgress)return false;

	if (!m_bCompleteLoginProgress) return false;
	{
		boost::lock_guard<boost::mutex> guard(mtx_item_cancel);
		// ����в�ѯ���󣬳���
		TradeQuqueItemQueryPosition posi(instrument);
		std::vector<TradeQuqueItemQueryPosition>::iterator it = std::find(allTradeQueueItemQueryPositions[instrument].begin(), allTradeQueueItemQueryPositions[instrument].end(), posi);
		if (it == allTradeQueueItemQueryPositions[instrument].end()) {
			TradeQuqueItemCancelOrder item(instrument, session, frontid, orderref);
			allTradeQueueItemCancelOrders[instrument].push_back(item);
		}
		return true;
	}

	/*ResetTryTimes();
	ReqOrderAction(instrument,session,frontid,orderref);*/
}

void Trade::handle_wait_orderinsert(const boost::system::error_code& error,
	char ordertype, const char* _instrumentId, double price, int director, int offset, int volume)
{
	//�����ǰƷ�����е�ִ�в���
	if (price>0)
		RemoveTradeQueueItem(_instrumentId);// , price, director, offset, volume);
	else
		RemoveTradeQueueItem(_instrumentId);// , -1, director, offset, volume);

	if (!error) { // not error: ��ʱ�ˣ������ǹҵ���ʱ��Ҳ�����ǳɽ���ʱ
		m_bRequestInProgress = false;
	}
	return;

	if (error)
	{
		// ɾ����һ�����Ƚ��бȽ�
		return;
	}

	//if (m_nTryTimes > MAX_REQUEST_TRY_TIMES)
	{
		m_bRequestInProgress = false;
		return;
	}

	//IncreaseTryTimes();
	
	if (ordertype == THOST_FTDC_OPT_LimitPrice)
	{
		ReqOrderInsert(_instrumentId, price, director, offset, volume);
	}
	else
	{
		ReqOrderInsert(_instrumentId, director, offset, volume);
	}
}
//����-�޼�
int Trade::ReqOrderInsert(const char* instrument, double price, int director, int offset, int volume)
{
	if (!m_bCompleteLoginProgress) {
		ClearTradeQueueItem();
		return -1;
	}
	if (m_bRequestInProgress)
	{
		return -1;
	}

	m_bRequestInProgress = true;

	// ���͵�redis
	message = "ReqOrderInsert_start";
	redis_worker.update_ctp_account_login_info(TradeContext::get_mutable_instance().argus.get_username().c_str(), get_trade_status_json_string().c_str());

	CThostFtdcInputOrderField f;
	memset(&f, 0, sizeof(f));
	f.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;	//1Ͷ��
	f.ContingentCondition = THOST_FTDC_CC_Immediately;//��������
	f.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	f.IsAutoSuspend = 0;
	f.OrderPriceType = THOST_FTDC_OPT_LimitPrice;		//***�����1  �޼�2***
	f.TimeCondition = THOST_FTDC_TC_GFD;		//***�������1  ������Ч3***
	f.VolumeCondition = THOST_FTDC_VC_AV;	//��������1  ��С����2  ȫ���ɽ�3
	f.MinVolume = 1;

	strcpy(f.InvestorID, investor);
	strcpy(f.UserID, investor);
	strcpy(f.BrokerID, broker);

	strcpy(f.InstrumentID, instrument);	//��Լ

	f.LimitPrice = price;				//***�۸�***

	if (director == 0)
		f.Direction = THOST_FTDC_D_Buy;			//��
	else
		f.Direction = THOST_FTDC_D_Sell;			//��

	if (offset == 0)
		f.CombOffsetFlag[0] = THOST_FTDC_OF_Open;//����
	else if (offset == 1)
		f.CombOffsetFlag[0] = THOST_FTDC_OF_Close;	//ƽ��
	else
		f.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;	//ƽ��

	f.VolumeTotalOriginal = volume;//����

	std::string temp = str(boost::format("%1%%2%") %++orderRef %current_guid);

	sprintf(f.OrderRef, "%s", temp.substr(0, 12).c_str());//OrderRef

	//���浱ǰorderref
	SetTradeQueueItemOrderRef(instrument, price, director, offset, volume, f.OrderRef);

	LOG(INFO)<<str(boost::format("����(���%1%).....") % f.OrderRef);

	int counter = 0;
	int nRet = pUserApi->ReqOrderInsert(&f, iReqID++);	//����
	while (nRet != 0 && (counter++)<50)
	{
		boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(20));
		nRet = pUserApi->ReqOrderInsert(&f, iReqID++);	//����
	}
	if (nRet == 0)
	{
		//�ܹ��ȴ�2��ʱ�䣬�ӱ������ɽ�
		orderinsertTime->expires_from_now(boost::posix_time::seconds(2));
		orderinsertTime->async_wait(boost::bind(&Trade::handle_wait_orderinsert,
			this,
			boost::asio::placeholders::error,
			THOST_FTDC_OPT_LimitPrice,
			instrument,
			price,
			director,
			offset,
			volume));
	}
	else
	{
		m_bRequestInProgress = false;
		// ���͵�redis
		message = "ReqOrderInsert_failed_"+nRet;
		redis_worker.update_ctp_account_login_info(TradeContext::get_mutable_instance().argus.get_username().c_str(), get_trade_status_json_string().c_str());
	}
	return nRet;
}
//����-�м�
int Trade::ReqOrderInsert(const char* instrument, int director, int offset, int volume)
{
	if (!m_bCompleteLoginProgress) {
		ClearTradeQueueItem();
		return -1;
	}

	if (m_bRequestInProgress)
	{
		return -1;
	}
	m_bRequestInProgress = true;
	// ���͵�redis
	message = "ReqOrderInsert_start";
	redis_worker.update_ctp_account_login_info(TradeContext::get_mutable_instance().argus.get_username().c_str(), get_trade_status_json_string().c_str());

	CThostFtdcInputOrderField f;
	memset(&f, 0, sizeof(f));
	f.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;	//1Ͷ��
	f.ContingentCondition = THOST_FTDC_CC_Immediately;//��������
	f.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	f.IsAutoSuspend = 0;
	f.OrderPriceType = THOST_FTDC_OPT_AnyPrice;		//***�����1  �޼�2***
	f.TimeCondition = THOST_FTDC_TC_IOC;		//***�������1  ������Ч3***
	f.VolumeCondition = THOST_FTDC_VC_AV;	//��������1  ��С����2  ȫ���ɽ�3
	f.MinVolume = 1;

	strcpy(f.InvestorID, investor);
	strcpy(f.UserID, investor);
	strcpy(f.BrokerID, broker);

	strcpy(f.InstrumentID, instrument);	//��Լ

	f.LimitPrice = 0;					//***�۸�***

	if (director == 0)
		f.Direction = THOST_FTDC_D_Buy;			//��
	else
		f.Direction = THOST_FTDC_D_Sell;			//��

	if (offset == 0)
		f.CombOffsetFlag[0] = THOST_FTDC_OF_Open;//����
	else if (offset == 1)
		f.CombOffsetFlag[0] = THOST_FTDC_OF_Close;	//ƽ��
	else
		f.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;	//ƽ��

	f.VolumeTotalOriginal = volume;//����
	std::string temp = str(boost::format("%1%%2%") % ++orderRef %current_guid);

	sprintf(f.OrderRef, "%s",temp.substr(0,12).c_str());//OrderRef

	LOG(INFO) << (str(boost::format("����(���%1%).....") % f.OrderRef));
	//���浱ǰorderref
	SetTradeQueueItemOrderRef(instrument, -1, director, offset, volume, f.OrderRef);

	int counter = 0;
	int nRet = pUserApi->ReqOrderInsert(&f, iReqID++);	//����
	while (nRet != 0 && (counter++)<50)
	{
		boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(20));
		nRet = pUserApi->ReqOrderInsert(&f, iReqID++);	//����
	}

	if (nRet == 0)
	{
		//�ܹ��ȴ�2��ʱ�䣬�ӱ������ɽ�
		orderinsertTime->expires_from_now(boost::posix_time::seconds(2));
		orderinsertTime->async_wait(boost::bind(&Trade::handle_wait_orderinsert,
			this,
			boost::asio::placeholders::error,
			THOST_FTDC_OPT_AnyPrice,
			instrument,
			0,
			director,
			offset,
			volume));
	}
	else
	{
		m_bRequestInProgress = false;

		// ���͵�redis
		message = "ReqOrderInsert_failed_" + nRet;
		redis_worker.update_ctp_account_login_info(TradeContext::get_mutable_instance().argus.get_username().c_str(), get_trade_status_json_string().c_str());
	}
	return nRet;
}
//��������,��CRI����ʱ������ʹ��
void Trade::pushOrders(int ticks, int seconds, const char* instrument, double price, int director, int offset, int volume)
{
	int slp = 1000 / ticks;
	for (int i = 0; i < ticks * seconds; ++i)
	{
		ReqOrderInsert(instrument, price, director, offset, volume);
		boost::this_thread::sleep(boost::posix_time::millisec(slp));
	}
}
//������Ӧ
void Trade::OnRtnOrder(CThostFtdcOrderField* pOrder)
{
	LOG(INFO) << "������Ӧ(" << pOrder->InstrumentID << "):" << pOrder->StatusMsg;
	//int thisOrderRef = boost::lexical_cast<int>(pOrder->OrderRef);
	//if (thisOrderRef > orderRefInitValue && thisOrderRef <= orderRef)
	{
		// ��ʱֻ���� ��ǰ�״̬�ı�����Ӧ
		TradeContext::get_mutable_instance().InsertCThostFtdcOrderField(pOrder);
		//���µ�ǰ��Լ�ı�����Ӧ,���ܻ�����������
		redis_worker.update_ctp_CThostFtdcOrderField(ctp_message_def::KEY_ACCOUNT_CTHOSTFTDCORDERFIELD_SUFFIX, investor, pOrder->InstrumentID, TradeContext::get_mutable_instance().m_allInstrumentRuntimes[pOrder->InstrumentID].ftdcOrderFields);
		redis_worker.update_ctp_CThostFtdcOrderField(ctp_message_def::KEY_ACCOUNT_PENDING_CTHOSTFTDCORDERFIELD_SUFFIX, investor, pOrder->InstrumentID, TradeContext::get_mutable_instance().m_allInstrumentRuntimes[pOrder->InstrumentID].pengdingFtdcOrderFields);
	}
	if (m_bCompleteLoginProgress)
	{
		
		// ��Ȼ����Ӧ�ˣ��Ǿʹ��б���ɾ��
		//RemoveTradeQueueItem(pOrder->InstrumentID, pOrder->OrderRef);
		RemoveTradeQueueItem(pOrder->InstrumentID);
		if (pOrder->OrderSubmitStatus == THOST_FTDC_OSS_InsertRejected
			//|| pOrder->OrderStatus== THOST_FTDC_OST_NoTradeNotQueueing
			|| pOrder->OrderStatus == THOST_FTDC_OST_Canceled)
		{
			//m_bRequestInProgress = false;
			// ���͵�redis
			message = "ReqOrderInsert_error_" + THOST_FTDC_OSS_InsertRejected;
			redis_worker.update_ctp_account_login_info(TradeContext::get_mutable_instance().argus.get_username().c_str(), get_trade_status_json_string().c_str());
			
			//�������ܾ������óɿ��Լ�����������ȡ����ʱ��
			orderinsertTime->cancel();
			m_bRequestInProgress = false;
		}
		//else {
		//	if (m_bRequestInProgress) {
		//		//�ȴ�1.5��ɽ����������ȡ���ҵ������±���
		//	}
		//}
	}
}
//��������
void Trade::OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	LOG(ERROR) << "��������("<<pInputOrder->InstrumentID<<"):" << (pRspInfo != nullptr ? pRspInfo->ErrorMsg : "unknown error");
	
	if (pInputOrder != nullptr) {
		// ��Ȼ����Ӧ�ˣ��Ǿʹ��б���ɾ��
		//RemoveTradeQueueItem(pInputOrder->InstrumentID, pInputOrder->OrderRef);
		RemoveTradeQueueItem(pInputOrder->InstrumentID);
	}

	m_bRequestInProgress = false;
	if (m_bCompleteLoginProgress)
	{
		//����ȡ��
		orderinsertTime->cancel();
		// ���͵�redis
		message = "ReqOrderInsert_error_" + (pRspInfo == nullptr ? 9999 : pRspInfo->ErrorID);
		redis_worker.update_ctp_account_login_info(TradeContext::get_mutable_instance().argus.get_username().c_str(), get_trade_status_json_string().c_str());
	}

	if (pInputOrder == nullptr) return;

	
	//int thisOrderRef = boost::lexical_cast<int>(pInputOrder->OrderRef);
	//if (thisOrderRef > orderRefInitValue && thisOrderRef <= orderRef)
	{
		// ��ʱֻ����״̬��
		TradeContext::get_mutable_instance().InsertCThostFtdcInputOrderField(pInputOrder);

		//���µ�ǰ��Լ�ı�����Ӧ
		redis_worker.update_ctp_CThostFtdcInputOrderField(investor, pInputOrder->InstrumentID, TradeContext::get_mutable_instance().m_allInstrumentRuntimes[pInputOrder->InstrumentID].ftdcInputOrderFields);
	}
}

//��������
void Trade::OnErrRtnOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo)
{
	LOG(ERROR) << "��������(" << pInputOrder->InstrumentID << "):" <<  (pRspInfo != nullptr?pRspInfo->ErrorMsg:"unknown error");

	if (pInputOrder != nullptr) {
		// ��Ȼ����Ӧ�ˣ��Ǿʹ��б���ɾ��
		//RemoveTradeQueueItem(pInputOrder->InstrumentID, pInputOrder->OrderRef);
		RemoveTradeQueueItem(pInputOrder->InstrumentID);
	}
	//����������󣬿��Խ�����һ����ѯ���߲���
	m_bRequestInProgress = false;

	if (m_bCompleteLoginProgress)
	{
		//����ȡ��
		orderinsertTime->cancel();

		message = "ReqOrderInsert_error_" + (pRspInfo == nullptr ? 9999: pRspInfo->ErrorID);
		redis_worker.update_ctp_account_login_info(TradeContext::get_mutable_instance().argus.get_username().c_str(), get_trade_status_json_string().c_str());
	}
	
	
	if (pInputOrder == nullptr) return;

	
	//int thisOrderRef = boost::lexical_cast<int>(pInputOrder->OrderRef);
	//if (thisOrderRef > orderRefInitValue && thisOrderRef <= orderRef)
	{
		// ��ʱֻ����״̬��
		TradeContext::get_mutable_instance().InsertCThostFtdcInputOrderField(pInputOrder);

		redis_worker.update_ctp_CThostFtdcInputOrderField(investor, pInputOrder->InstrumentID, TradeContext::get_mutable_instance().m_allInstrumentRuntimes[pInputOrder->InstrumentID].ftdcInputOrderFields);
	}
}

/********************���� block************************************/
/********************����************************************/
void Trade::handle_wait_orderaction(const boost::system::error_code & error, const char * instrument, int session, int frontid, const char * orderref)
{
	m_bRequestInProgress = false;
	RemoveTradeQueueItem(instrument, session, frontid, orderref);
	return;
	if (error) return;

	if (m_nTryTimes > MAX_REQUEST_TRY_TIMES)
	{
		m_bRequestInProgress = false;
		return;
	}

	// ��һ�β�ѯ
	//IncreaseTryTimes();
	ReqOrderAction(instrument,session,frontid,orderref);
}
void Trade::ReqOrderAction(const char* instrument, int session, int frontid, const char* orderref)
{
	if (!m_bCompleteLoginProgress) {
		RemoveTradeQueueItem(instrument);
		RemoveTradeQueueItem(instrument, session, frontid, orderref);
		return;
	}

	m_bRequestInProgress = true;
	bool bFind = false;
	//�Ȳ���
	for (size_t i = 0; i < TradeContext::get_mutable_instance().m_allInstrumentRuntimes[instrument].pengdingFtdcOrderFields.size(); ++i)
	{
		MyCThostFtdcOrderField &field = TradeContext::get_mutable_instance().m_allInstrumentRuntimes[instrument].pengdingFtdcOrderFields[i];
		if (strcmp(field._field.InstrumentID, instrument) == 0
			&& field._field.SessionID == session
			&& field._field.FrontID == frontid
			&& strcmp(field._field.OrderRef, orderref) == 0)
		{
			bFind = true;
			break;
		}
	}
	
	if (!bFind) {
		LOG(INFO) << "�ҵ��Ѿ���ȡ��������Ҫ�Զ�ȡ��";
		RemoveTradeQueueItem(instrument, session, frontid, orderref);
		m_bRequestInProgress = false;
		return;
	}
	
	message = "ReqOrderAction_start";
	redis_worker.update_ctp_account_login_info(TradeContext::get_mutable_instance().argus.get_username().c_str(), get_trade_status_json_string().c_str());

	CThostFtdcInputOrderActionField f;
	memset(&f, 0, sizeof(f));
	f.ActionFlag = THOST_FTDC_AF_Delete; 	//THOST_FTDC_AF_Modify
	strcpy(f.BrokerID, broker);
	strcpy(f.InvestorID, investor);

	strcpy(f.InstrumentID, instrument);
	f.SessionID = session;
	f.FrontID = frontid;
	strcpy(f.OrderRef, orderref);



	int counter = 0;
	int nRet = pUserApi->ReqOrderAction(&f, ++iReqID);
	while (nRet != 0 && (counter++)<50)
	{
		boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(20));
		nRet = pUserApi->ReqOrderAction(&f, ++iReqID);
	}

	if (nRet != 0)
	{
		m_bRequestInProgress = false;
		message = "ReqOrderAction_failed_"+nRet;
		redis_worker.update_ctp_account_login_info(TradeContext::get_mutable_instance().argus.get_username().c_str(), get_trade_status_json_string().c_str());
		return;
	}

	orderactionTime->expires_from_now(boost::posix_time::seconds(3));
	orderactionTime->async_wait(boost::bind(&Trade::handle_wait_orderaction,
		this,
		boost::asio::placeholders::error,
		instrument,
		session,
		frontid,
		orderref));
}
//�������
void Trade::OnRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (pInputOrderAction == nullptr) return;
	if (!IsErrorRspInfo(pRspInfo))
	{
		TradeContext::get_mutable_instance().HandleRspOrderAction(pInputOrderAction, pRspInfo, nRequestID, bIsLast);
	}
	
	

	if(RemoveTradeQueueItem(pInputOrderAction->InstrumentID, pInputOrderAction->SessionID, pInputOrderAction->FrontID, pInputOrderAction->OrderRef))
		m_bRequestInProgress = false;

	message = "ReqOrderAction_end";
	redis_worker.update_ctp_account_login_info(TradeContext::get_mutable_instance().argus.get_username().c_str(), get_trade_status_json_string().c_str());
}

/********************���� block************************************/
//��ֲ���ϸ�� TODO����ʱҲ���ż���
void Trade::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField* pInvestorPositionDetail,
	CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (pInvestorPositionDetail)
	{
		
	}
	if (bIsLast)
	{
		
	}
}

void Trade::handle_wait_status_update(const boost::system::error_code & error)
{
	if (!m_bRequestInProgress)
	{
		CThostFtdcQryTradingAccountField pQryTradingAccount = { 0 };
		strncpy(pQryTradingAccount.BrokerID, broker, strlen(broker));
		strncpy(pQryTradingAccount.InvestorID, investor, strlen(investor));
		pUserApi->ReqQryTradingAccount(&pQryTradingAccount, ++iReqID);
	}

	tradeStatusUpdateTimer.expires_from_now(boost::posix_time::seconds(2));
	tradeStatusUpdateTimer.async_wait(boost::bind(&Trade::handle_wait_status_update, this, boost::asio::placeholders::error));
	
}
//���ʽ�
void Trade::OnRspQryTradingAccount(CThostFtdcTradingAccountField* pTradingAccount,
	CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	//if (bIsLast)
	//{
	//	TradeContext::get_mutable_instance().pTradingAccount = pTradingAccount;
	//	//��̬Ȩ��=���ս���-������+�����
	//	TradeContext::get_mutable_instance().preBalance = pTradingAccount->PreBalance - pTradingAccount->Withdraw + pTradingAccount->Deposit;
	//	//��̬Ȩ��=��̬Ȩ��+ ƽ��ӯ��+ �ֲ�ӯ��- ������
	//	TradeContext::get_mutable_instance().currentBalance = TradeContext::get_mutable_instance().preBalance
	//		+ pTradingAccount->CloseProfit + pTradingAccount->PositionProfit - pTradingAccount->Commission;
	//}


	redis_worker.update_ctp_account_info(*pTradingAccount);
	
}



//�ɽ���Ӧ
void Trade::OnRtnTrade(CThostFtdcTradeField* pTrade)
{
	if (pTrade == NULL) return;
	//int thisOrderRef = boost::lexical_cast<int>(pTrade->OrderRef);
	//if (thisOrderRef > orderRefInitValue && thisOrderRef <= orderRef)
	{
		// ��ʱֻ���� ��ǰ�״̬�ĳɽ���Ӧ
		TradeContext::get_mutable_instance().InsertCThostFtdcTradeField(pTrade);
	}
	//�����λ�仯����Ҫ���²�ѯ,���������ζ��������²�ѯ�أ������԰�
	//if (m_bQryInverstorPositionAfterTrade)
	if(m_bCompleteLoginProgress)
	{
		// һ�ιҵ����ֵ�ʱ�򣬳ɽ����ܺܿ죬���ǳɽ���Ӧ�����Ƕ�Σ���������ʱ
		// �����͵��� m_bRequestInProgress ����false���ڶ��γɽ���Ӧ�յ����ٴ�ִ�в�ѯ���Ѹñ����ó�true
		/*if (!m_bRequestInProgress)
		{
			m_bRequestInProgress = true;
			
		}*/

		
		//ResetTryTimes();
		//TradeContext::get_mutable_instance().ClearInstrumentInvestorPosition(pTrade->InstrumentID);
		//ReqQryInvestorPosition(pTrade->InstrumentID);
		//����Ƿ����ɽ��ģ������Ǳ�ĳ����ύ�ģ��������һ��취��û�У�
		m_bRequestInProgress = true;//����ϱ�Ľ��а�
		//����ط������ȰѲ�ѯ����Ŷ��У�Ȼ������trade����ģʽ������ִ�в�ѯ����
		reqQryInvestorPositionInQueue(pTrade->InstrumentID);

		//ֻ�е��ɽ��󣬲���Ϊ������ƽ���̽���
		orderinsertTime->cancel();
		// ���Բ�ѯ��
		m_bRequestInProgress = false;
	}
	// ���³ɽ���Ӧ
	redis_worker.update_ctp_CThostFtdcTradeField(investor, pTrade->InstrumentID, TradeContext::get_mutable_instance().m_allInstrumentRuntimes[pTrade->InstrumentID].ftdcTradeFields);
	
}




//ǩԼ����
void Trade::OnRspQryAccountregister(CThostFtdcAccountregisterField *pAccountregister, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	//if (IsErrorRspInfo(pRspInfo))
	//	;//CMyLogger::instance().info(str(boost::format("��ǩԼ���д���:%1%") % pRspInfo->ErrorMsg));
	//else if (pAccountregister)
	//{
	//	string bank = "";
	//	if (pAccountregister->BankID[0] == THOST_FTDC_BF_ABC)
	//		bank = "ũҵ����";
	//	else if (pAccountregister->BankID[0] == THOST_FTDC_BF_BC)
	//		bank = "�й�����";
	//	else if (pAccountregister->BankID[0] == THOST_FTDC_BF_BOC)
	//		bank = "��ͨ����";
	//	else if (pAccountregister->BankID[0] == THOST_FTDC_BF_CBC)
	//		bank = "��������";
	//	else if (pAccountregister->BankID[0] == THOST_FTDC_BF_ICBC)
	//		bank = "��������";
	//	else if (pAccountregister->BankID[0] == THOST_FTDC_BF_Other)
	//		bank = "��������";

	//	string bankID = string(pAccountregister->BankAccount);
	//	bankID = bankID.substr(strlen(pAccountregister->BankAccount) - 4, 4);
	//	//CMyLogger::instance().info(str(boost::format("%1%.%2%(****%3%)") % pAccountregister->BankID[0] % bank
	//	//%bankID));
	//}
	//else
	//	;//CMyLogger::instance().info("��ǩԼ����!");
}

//�������ʻ���Ӧ
void Trade::OnRtnQueryBankBalanceByFuture(CThostFtdcNotifyQueryAccountField* pNotifyQueryAccount)
{
	//CMyLogger::instance().info(str(boost::format("�������:%1%, ��ȡ���:%2%") % pNotifyQueryAccount->BankUseAmount
		//%pNotifyQueryAccount->BankFetchAmount));
}

//�������ʻ�����
void Trade::OnRspQueryBankAccountMoneyByFuture(CThostFtdcReqQueryAccountField* pReqQueryAccount, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (IsErrorRspInfo(pRspInfo))
		;//CMyLogger::instance().info(str(boost::format("��ѯ�����ʺŴ���:%1%") % pRspInfo->ErrorMsg));
	else
		;//CMyLogger::instance().info(pReqQueryAccount->Digest);
}

//�������ʻ�����
void Trade::OnErrRtnQueryBankBalanceByFuture(CThostFtdcReqQueryAccountField* pReqQueryAccount, CThostFtdcRspInfoField* pRspInfo)
{
	//CMyLogger::instance().info(str(boost::format("������������:%1%") % pRspInfo->ErrorMsg));
}

//��->��
void Trade::OnRtnFromFutureToBankByFuture(CThostFtdcRspTransferField* pRspTransfer)
{
	if (pRspTransfer->ErrorID == 0)
		;//CMyLogger::instance().info("�ڻ�ת���гɹ�!");
	else
		;//CMyLogger::instance().info(str(boost::format("�ڻ�ת����ʧ��:%1%") % pRspTransfer->ErrorMsg));
}

//��->������
void Trade::OnErrRtnFutureToBankByFuture(CThostFtdcReqTransferField* pReqTransfer, CThostFtdcRspInfoField* pRspInfo)
{
	//CMyLogger::instance().info(str(boost::format("�ڻ�ת����ʧ��:%1%") % pRspInfo->ErrorMsg));
}

//��->������
void Trade::OnRspFromFutureToBankByFuture(CThostFtdcReqTransferField* pReqTransfer, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	//CMyLogger::instance().info(str(boost::format("�ڻ�ת����ʧ��:%1%") % pRspInfo->ErrorMsg));
}

//��->��
void Trade::OnRtnFromBankToFutureByFuture(CThostFtdcRspTransferField* pRspTransfer)
{
	if (pRspTransfer->ErrorID == 0)
		;//CMyLogger::instance().info("����ת�ڻ��ɹ�!");
	else
		;//CMyLogger::instance().info(str(boost::format("����ת�ڻ�ʧ��:%1%") % pRspTransfer->ErrorMsg));
}

//��->�ڴ���
void Trade::OnErrRtnBankToFutureByFuture(CThostFtdcReqTransferField* pReqTransfer, CThostFtdcRspInfoField* pRspInfo)
{
	//CMyLogger::instance().info(str(boost::format("����ת�ڻ�ʧ��:%1%") % pRspInfo->ErrorMsg));
}

//��->�ڴ���
void Trade::OnRspFromBankToFutureByFuture(CThostFtdcReqTransferField* pReqTransfer, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	//CMyLogger::instance().info(str(boost::format("����ת�ڻ�ʧ��:%1%") % pRspInfo->ErrorMsg));
}

//(*������Ӧ�¼�
void Trade::OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	m_bCompleteLoginProgress = false;
}

void Trade::OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField* pUserPasswordUpdate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspTradingAccountPasswordUpdate(CThostFtdcTradingAccountPasswordUpdateField* pTradingAccountPasswordUpdate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspParkedOrderInsert(CThostFtdcParkedOrderField* pParkedOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspParkedOrderAction(CThostFtdcParkedOrderActionField* pParkedOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspQueryMaxOrderVolume(CThostFtdcQueryMaxOrderVolumeField* pQueryMaxOrderVolume, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspRemoveParkedOrder(CThostFtdcRemoveParkedOrderField* pRemoveParkedOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspRemoveParkedOrderAction(CThostFtdcRemoveParkedOrderActionField* pRemoveParkedOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspQryOrder(CThostFtdcOrderField* pOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspQryTrade(CThostFtdcTradeField* pTrade, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspQryInvestor(CThostFtdcInvestorField* pInvestor, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspQryTradingCode(CThostFtdcTradingCodeField* pTradingCode, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField* pInstrumentMarginRate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField* pInstrumentCommissionRate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspQryExchange(CThostFtdcExchangeField* pExchange, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspQryTransferBank(CThostFtdcTransferBankField* pTransferBank, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspQryNotice(CThostFtdcNoticeField* pNotice, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspQryInvestorPositionCombineDetail(CThostFtdcInvestorPositionCombineDetailField* pInvestorPositionCombineDetail, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspQryCFMMCTradingAccountKey(CThostFtdcCFMMCTradingAccountKeyField* pCFMMCTradingAccountKey, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspQryEWarrantOffset(CThostFtdcEWarrantOffsetField* pEWarrantOffset, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspQryTransferSerial(CThostFtdcTransferSerialField* pTransferSerial, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	int i = 0;
}

void Trade::OnErrRtnOrderAction(CThostFtdcOrderActionField* pOrderAction, CThostFtdcRspInfoField* pRspInfo)
{
}

void Trade::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField* pInstrumentStatus)
{
}

void Trade::OnRtnTradingNotice(CThostFtdcTradingNoticeInfoField* pTradingNoticeInfo)
{
}

void Trade::OnRtnErrorConditionalOrder(CThostFtdcErrorConditionalOrderField* pErrorConditionalOrder)
{
}

void Trade::OnRspQryContractBank(CThostFtdcContractBankField* pContractBank, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspQryParkedOrder(CThostFtdcParkedOrderField* pParkedOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspQryParkedOrderAction(CThostFtdcParkedOrderActionField* pParkedOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspQryTradingNotice(CThostFtdcTradingNoticeField* pTradingNotice, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspQryBrokerTradingParams(CThostFtdcBrokerTradingParamsField* pBrokerTradingParams, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRspQryBrokerTradingAlgos(CThostFtdcBrokerTradingAlgosField* pBrokerTradingAlgos, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void Trade::OnRtnFromBankToFutureByBank(CThostFtdcRspTransferField* pRspTransfer)
{
}

void Trade::OnRtnFromFutureToBankByBank(CThostFtdcRspTransferField* pRspTransfer)
{
}

void Trade::OnRtnRepealFromBankToFutureByBank(CThostFtdcRspRepealField* pRspRepeal)
{
}

void Trade::OnRtnRepealFromFutureToBankByBank(CThostFtdcRspRepealField* pRspRepeal)
{
}

void Trade::OnRtnRepealFromBankToFutureByFutureManual(CThostFtdcRspRepealField* pRspRepeal)
{
}

void Trade::OnRtnRepealFromFutureToBankByFutureManual(CThostFtdcRspRepealField* pRspRepeal)
{
}

void Trade::OnErrRtnRepealBankToFutureByFutureManual(CThostFtdcReqRepealField* pReqRepeal, CThostFtdcRspInfoField* pRspInfo)
{
}

void Trade::OnErrRtnRepealFutureToBankByFutureManual(CThostFtdcReqRepealField* pReqRepeal, CThostFtdcRspInfoField* pRspInfo)
{
}

void Trade::OnRtnRepealFromBankToFutureByFuture(CThostFtdcRspRepealField* pRspRepeal)
{
}

void Trade::OnRtnRepealFromFutureToBankByFuture(CThostFtdcRspRepealField* pRspRepeal)
{
}

void Trade::OnRtnOpenAccountByBank(CThostFtdcOpenAccountField* pOpenAccount)
{
}

void Trade::OnRtnCancelAccountByBank(CThostFtdcCancelAccountField* pCancelAccount)
{
}

void Trade::OnRtnChangeAccountByBank(CThostFtdcChangeAccountField* pChangeAccount)
{
}
//*)

//void Quote::handle_wait_connect(const boost::system::error_code& error)
//{
//	if (error) return; // ����0��ʾcancel��������995
//					   // error ��������
//	if (m_bFrontConnected) return;
//
//	if (m_nTryTimes > MAX_REQUEST_TRY_TIMES)
//	{
//		LOG(ERROR) << MAX_REQUEST_TRY_TIMES << " times of front connect reached";
//		actionTimer.get_io_service().stop();
//		return;
//	}
//	LOG(WARNING) << "Connect failed, trying times:" << m_nTryTimes;
//	IncreaseTryTimes();
//
//	Connect();
//}
//void Quote::Connect()
//{
//	LOG(INFO) << "ReqConnecting, try times = " << m_nTryTimes;
//	ReqConnect((char*)TradeContext::get_mutable_instance().argus.get_addresses()[0].c_str(),
//		(char*) TradeContext::get_mutable_instance().argus.get_broker().c_str());
//	actionTimer.expires_from_now(boost::posix_time::seconds(10));
//	actionTimer.async_wait(boost::bind(&Quote::handle_wait_connect, this, boost::asio::placeholders::error));
//}
//void Quote::ReqConnect(char* addr, char* brokerID)
//{
//	strcpy(this->frontAddr, addr);
//	strcpy(this->broker, brokerID);
//	if (pUserApi == NULL)
//	{
//		pUserApi = CThostFtdcMdApi::CreateFtdcMdApi("quote", false);
//	}
//	else {
//		pUserApi->Release();
//		pUserApi = CThostFtdcMdApi::CreateFtdcMdApi("quote", false);
//	}
//	pUserApi->RegisterSpi(this);
//	pUserApi->RegisterFront(frontAddr);
//
//	pUserApi->Init();
//}
////(*������Ӧ
//
//void Quote::OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
//{
//}
//
//void Quote::OnFrontConnected()
//{
//	actionTimer.cancel();
//	m_bFrontConnected = true;
//	ResetTryTimes();
//	this->ReqLogin(user, pwd);
//}
//
//void Quote::Login()
//{
//	ReqLogin(TradeContext::get_mutable_instance().argus.get_username().c_str()
//		, TradeContext::get_mutable_instance().argus.get_password().c_str());
//
//	status = STATUS_USER_LOGINING;
//	message = boost::str(boost::format("ReqLogining, try times = %1%") % m_nTryTimes);
//	//redis_worker.update_ctp_account_login_info(investor, get_trade_status_json_string().c_str());
//	LOG(INFO) << message;
//
//	actionTimer.expires_from_now(boost::posix_time::seconds(5));
//	actionTimer.async_wait(boost::bind(&Quote::handle_wait_login, this, boost::asio::placeholders::error));
//}
//
//void Quote::handle_wait_login(const boost::system::error_code& error)
//{
//	if (m_bUserLogin) return;
//
//	if (m_nTryTimes > MAX_REQUEST_TRY_TIMES)
//	{
//		status = STATUS_USER_LOGIN_FAIL;
//		message = "User Login failed after 5 try times......";
//		actionTimer.get_io_service().stop();
//		return;
//	}
//
//	IncreaseTryTimes();
//
//	Login();
//}
//void Quote::OnFrontDisconnected(int nReason)
//{
//	m_bFrontConnected = false;
//}
//
//void Quote::OnHeartBeatWarning(int nTimeLapse)
//{
//}
//
//void Quote::get_redis_ctp_instrument_ids_handler(const RedisValue &root)
//{
//	if (root.isString())
//	{
//		Json::Reader reader;
//		Json::Value value;
//		if (reader.parse(root.toString(), value))
//		{
//			if (value.isArray())
//			{
//				char* tmp[1] = { new char[256] };
//				for (int i = 0; i < value.size(); ++i)
//				{
//					memset(tmp[0], 0, 256);
//					const char *ids = value[i].asCString();
//					strncpy(tmp[0],ids, strlen(ids) );
//					LOG(INFO) << "SubscribeMarketData " << ids;
//					pUserApi->SubscribeMarketData(tmp, 1);
//				}
//				delete tmp[0];
//			}
//		}
//	}
//}
//void Quote::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin,
//	CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
//{
//	actionTimer.cancel();
//	if (IsErrorRspInfo(pRspInfo))
//	{
//		m_bUserLogin = false;
//	}
//	else
//	{
//		m_bUserLogin = true;
//		LOG(INFO) << "login done";
//		
//		redis_worker.get_ctp_instrument_ids(boost::bind(&Quote::get_redis_ctp_instrument_ids_handler, this, _1));
//		//pUserApi->SubscribeMarketData(nullptr, 0);
//		//map<string, CTradeItem>::iterator it;
//
//		//BOOST_FOREACH(mapInstrument::value_type i, dicInstrument)
//		/*for (it = TradeItemMgr::instance().m_allTradeItems.begin();
//			it != TradeItemMgr::instance().m_allTradeItems.end();
//			++it)
//		{
//			strcpy(tmp[0], it->first.c_str());
//			pUserApi->SubscribeMarketData(tmp, 1);
//		}*/
//		
//	}
//}
//
//void Quote::OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
//{
//	if (IsErrorRspInfo(pRspInfo))
//	{
//		//CMyLogger::instance().info(str(boost::format("���鶩�Ĵ���:%1%") % pRspInfo->ErrorMsg));
//	}
//	else
//	{
//
//	}
//}
//
//void Quote::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
//{
//}
////*)
//
////���������Ӧ
//void Quote::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData)
//{
//	int i = 0;
//
//}

//tickд���ı�
void Trade::tickToFile(CThostFtdcDepthMarketDataField f)
{
	return;
}






//��ֲ�

////��ֲ�
//void Trade::ReqQryInvestorPosition(TThostFtdcInstrumentIDType &_instrumentID)
//{
//	//if (m_bStartQuery && !m_bQueryFinished)return;
//
//	m_bQueryFinished = false;
//	m_bStartQuery = true;
//	//TradeItemMgr::instance().resetInvestorPosition();
//	boost::this_thread::sleep(boost::posix_time::millisec(1000));
//
//	CThostFtdcQryInvestorPositionField f;
//	memset(&f, 0, sizeof(f));
//	strncpy(f.InstrumentID, _instrumentID, 30);
//	strcpy(f.BrokerID, broker);
//	strcpy(f.InvestorID, investor);
//	pUserApi->ReqQryInvestorPosition(&f, ++iReqID);
//
//}

//��ֲ���ϸ
void Trade::ReqQryInvestorPositionDetail()
{
	boost::this_thread::sleep(boost::posix_time::millisec(1000));

	CThostFtdcQryInvestorPositionDetailField f;
	memset(&f, 0, sizeof(f));
	strcpy(f.BrokerID, broker);
	strcpy(f.InvestorID, investor);
	pUserApi->ReqQryInvestorPositionDetail(&f, ++iReqID);
}

const std::string Trade::get_trade_status_json_string()
{
	Json::Value val;
	std::string str = boost::str(boost::format("%1%_%2%_%3%") % "trade" %TradeContext::get_mutable_instance().argus.get_username() %my_utils::get_local_ip_address());
	val["trade_id"] = str;
	val["login_complete"] = m_bCompleteLoginProgress;
	val["status"] = status;
	val["msg"] = message;
	val["request_in_progress"] = m_bRequestInProgress;
	val["time"] = my_utils::get_local_time();
	return val.toStyledString();
}


void Trade::reqQryInvestorPositionInQueue(const char *_instrumentID)
{
	boost::lock_guard<boost::mutex> guard(mtx_item_query);
	//�Ȳ����ٲ��룬���ʱ��Ҫ��Ҫ�����û�п����Ĳ�����
	// ԭ�����������
	TradeQuqueItemQueryPosition item(_instrumentID);
	if (allTradeQueueItemQueryPositions.find(_instrumentID) == allTradeQueueItemQueryPositions.end()) {
		allTradeQueueItemQueryPositions[_instrumentID].push_back(item);
	}
	else {

		//����3721���Ž�ȥ����˼��˵��������γɽ��ģ�ִ�ж�β�ѯ
		//���Ҳ�ѯ����������ȼ�������Ӱ������
		allTradeQueueItemQueryPositions[_instrumentID].push_back(item);
		/*std::vector<TradeQuqueItemQueryPosition>::iterator it = std::find(allTradeQueueItemQueryPositions[_instrumentID].begin(), allTradeQueueItemQueryPositions[_instrumentID].end(), item);

		if (it == allTradeQueueItemQueryPositions[_instrumentID].end()) {
			allTradeQueueItemQueryPositions[_instrumentID].push_back(item);
		}*/
	}

	return;
	if (strlen(_instrumentID) == 0)return;
	// û�����ڲ�ѯ�ģ�ֱ�Ӳ�ѯ
	//if (!m_bRequestInProgress && m_currentQryInstrumentId.empty())
	//{
	//	LOG(DEBUG) << "reqQryInvestorPositionInQueue(ReqQryInvestorPosition):" << _instrumentID<<":Start";
	//	ResetTryTimes();
	//	// ���渳ֵ
	//	ReqQryInvestorPosition(_instrumentID);
	//	return;
	//}
	//else
	{
		{
			boost::lock_guard<boost::mutex> guard(pendingInstrumentIds_mutex);
			LOG(DEBUG) << "reqQryInvestorPositionInQueue(add to pending queue):" << _instrumentID;
			std::vector<std::string>::iterator it = std::find(allPendingQryInvestorPositions.begin(), allPendingQryInvestorPositions.end(), _instrumentID);
			if (it == allPendingQryInvestorPositions.end())
			{
				allPendingQryInvestorPositions.push_back(_instrumentID);
			}
		}
		//qryInvestorPositionTime->expires_from_now(boost::posix_time::seconds(1));
		//qryInvestorPositionTime->async_wait(boost::bind(&Trade::handle_queue_qryinvestorposition, this, boost::asio::placeholders::error));
		//���ʱ������һ��timer����ͣ����Ƿ���ҵ��ִ�У�û�еĻ��Ͳ�ѯ
	}
}