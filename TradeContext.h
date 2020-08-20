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
		//初始化很小的值
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
	/*记录某个品种的开仓情况
	*/
	void InsertCThostFtdcInvestorPositionField(CThostFtdcInvestorPositionField *pInvestorPosition)
	{
		if (pInvestorPosition == nullptr) return;
		m_allInstrumentRuntimes[pInvestorPosition->InstrumentID].InsertCThostFtdcInvestorPositionField(pInvestorPosition);
	}

	/*清除某个品种的开仓情况*/
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

	/*记录某个品种的报单情况
	*/
	void InsertCThostFtdcOrderField(CThostFtdcOrderField *pOrder)
	{
		if (pOrder == nullptr) return;
		m_allInstrumentRuntimes[pOrder->InstrumentID].InsertCThostFtdcOrderField(pOrder);
	}

	// 清除报单记录
	void ClearCThostFtdcOrderField(const char* instrumentID)
	{
		if (m_allInstrumentRuntimes.find(instrumentID) == m_allInstrumentRuntimes.end()) return;
		m_allInstrumentRuntimes[instrumentID].ClearCThostFtdcOrderField();
	}

	//插入成交相应
	void InsertCThostFtdcTradeField(CThostFtdcTradeField *pOrder)
	{
		if (m_allInstrumentRuntimes.find(pOrder->InstrumentID) == m_allInstrumentRuntimes.end()) return;
		m_allInstrumentRuntimes[pOrder->InstrumentID].InsertCThostFtdcTradeField(pOrder);
	}

	//清除所有的成交相应记录
	void ClearCThostFtdcTradeField(const char* instrumentID)
	{
		if (m_allInstrumentRuntimes.find(instrumentID) == m_allInstrumentRuntimes.end()) return;
		m_allInstrumentRuntimes[instrumentID].ClearCThostFtdcTradeField();
	}

	// 处理撤单响应
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
	//静态权益
	double preBalance;
	//动态权益
	double currentBalance;

	//
	int nBuyCount = 0;
	int nSellCount = 0;
};
#endif 