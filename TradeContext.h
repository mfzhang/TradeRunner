#ifndef TRADECONTEXT_H
#define TRADECONTEXT_H
#include <boost/serialization/singleton.hpp> 
#include "TradeRunnerArgus.h"
#include "QuoteRunnerArgus.h"
#include "InstrumentRuntime.h"

class TradeContext :public boost::serialization::singleton<TradeContext>
{
public:
	TradeContext()
	{
		pTradingAccount = nullptr;
		//��ʼ����С��ֵ
		preBalance = -100000;
		currentBalance = -100000;
	}

	TradeRunnerArgus argus;
	QuoteRunnerArgus quote_argus;
	std::map<std::string, InstrumentRuntime> m_allInstrumentRuntimes;

	void reset()
	{
		m_allInstrumentRuntimes.clear();
	}
	/*��¼ĳ��Ʒ�ֵĿ������
	*/
	void InsertCThostFtdcInvestorPositionField(CThostFtdcInvestorPositionField *pInvestorPosition)
	{
		if (pInvestorPosition == nullptr) return;
		m_allInstrumentRuntimes[pInvestorPosition->InstrumentID].InsertCThostFtdcInvestorPositionField(pInvestorPosition);
	}

	/*���ĳ��Ʒ�ֵĿ������*/
	void ClearInstrumentInvestorPosition(const char* instrumentID)
	{
		if (strlen(instrumentID) == 0) {

			std::map<std::string, InstrumentRuntime>::iterator it;
			for (it = m_allInstrumentRuntimes.begin(); it != m_allInstrumentRuntimes.end(); ++it) {
				it->second.ClearCThostFtdcInvestorPositionField();
			}
			return;
		}
		if (m_allInstrumentRuntimes.find(instrumentID) == m_allInstrumentRuntimes.end()) return;

		m_allInstrumentRuntimes[instrumentID].ClearCThostFtdcInvestorPositionField();
	}

	/*��¼ĳ��Ʒ�ֵı������
	*/
	void InsertCThostFtdcOrderField(CThostFtdcOrderField *pOrder)
	{
		if (pOrder == nullptr) return;
		m_allInstrumentRuntimes[pOrder->InstrumentID].InsertCThostFtdcOrderField(pOrder);
	}

	// ���������¼
	void ClearCThostFtdcOrderField(const char* instrumentID)
	{
		if (m_allInstrumentRuntimes.find(instrumentID) == m_allInstrumentRuntimes.end()) return;
		m_allInstrumentRuntimes[instrumentID].ClearCThostFtdcOrderField();
	}

	//����ɽ���Ӧ
	void InsertCThostFtdcTradeField(CThostFtdcTradeField *pOrder)
	{
		if (m_allInstrumentRuntimes.find(pOrder->InstrumentID) == m_allInstrumentRuntimes.end()) return;
		m_allInstrumentRuntimes[pOrder->InstrumentID].InsertCThostFtdcTradeField(pOrder);
	}

	//������еĳɽ���Ӧ��¼
	void ClearCThostFtdcTradeField(const char* instrumentID)
	{
		if (m_allInstrumentRuntimes.find(instrumentID) == m_allInstrumentRuntimes.end()) return;
		m_allInstrumentRuntimes[instrumentID].ClearCThostFtdcTradeField();
	}

	// ��������Ӧ
	void HandleRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (m_allInstrumentRuntimes.find(pInputOrderAction->InstrumentID) == m_allInstrumentRuntimes.end()) return;
		m_allInstrumentRuntimes[pInputOrderAction->InstrumentID].HandleRspOrderAction(pInputOrderAction, pRspInfo, nRequestID, bIsLast);
	}

	void InsertCThostFtdcInputOrderField(CThostFtdcInputOrderField *pField)
	{
		if (m_allInstrumentRuntimes.find(pField->InstrumentID) == m_allInstrumentRuntimes.end()) return;
		m_allInstrumentRuntimes[pField->InstrumentID].InsertCThostFtdcInputOrderField(pField);
	}
	CThostFtdcTradingAccountField *pTradingAccount;
	//��̬Ȩ��
	double preBalance;
	//��̬Ȩ��
	double currentBalance;

	//
	int nBuyCount = 0;
	int nSellCount = 0;
};
#endif 