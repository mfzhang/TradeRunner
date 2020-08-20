#ifndef INSTRUMENTRUNTIME_H__
#define INSTRUMENTRUNTIME_H__
#include <string>
#include <map>
#include <vector>
#include "ThostFtdcTraderApi.h"
typedef struct MyCThostFtdcOrderField {
	unsigned int _hashCode;
	CThostFtdcOrderField _field;

	void RecalculateHash();
}MyCThostFtdcOrderField;
//#include <boost/foreach.hpp>
typedef struct InstrumentSimpleValue {
	double open;
	double high;
	double low;
	double close;
	std::string tradingDay;
}InstrumentSimpleValue;

typedef std::map<TThostFtdcPosiDirectionType, std::vector<CThostFtdcInvestorPositionField>> TwoDirectionsCThostFtdcInvestorPositionFieldMap;

class InstrumentRuntime
{
public:
	std::string instrumentID;
	CThostFtdcInstrumentField _instrument;

	InstrumentSimpleValue latestValue;
	//两个方向的持仓
	
	TwoDirectionsCThostFtdcInvestorPositionFieldMap investorPositionAllDirections;
	typedef const TwoDirectionsCThostFtdcInvestorPositionFieldMap::value_type ConstPosiDirectionTypes;

	//报单相应,
	std::vector<MyCThostFtdcOrderField> ftdcOrderFields;
	//std::map<unsigned int,CThostFtdcOrderField> ftdcOrderFields;
	// pending报单,就是挂单,改成map吧，key是hash
	std::vector<MyCThostFtdcOrderField> pengdingFtdcOrderFields;
	//std::map<unsigned int, CThostFtdcOrderField> pengdingFtdcOrderFields;

	// 成交记录
	std::vector<CThostFtdcTradeField> ftdcTradeFields;

	// 报单错误
	std::vector<CThostFtdcInputOrderField> ftdcInputOrderFields;


	InstrumentRuntime();
	~InstrumentRuntime();

	// 插入持仓
	void InsertCThostFtdcInvestorPositionField(CThostFtdcInvestorPositionField *pInvestorPosition)
	{
		if (pInvestorPosition == nullptr) return;
		if (instrumentID.length() == 0)
		{
			instrumentID = pInvestorPosition->InstrumentID;
		}
		else if (instrumentID.compare(pInvestorPosition->InstrumentID) != 0)
		{
			return;
		}
		investorPositionAllDirections[pInvestorPosition->PosiDirection].push_back(*pInvestorPosition);
	}

	//清除持仓
	void ClearCThostFtdcInvestorPositionField()
	{
		investorPositionAllDirections.clear();
	}

	//插入报单,当前只存储pengding的
	void InsertCThostFtdcOrderField(CThostFtdcOrderField *pOrder);
	/*{
		MyCThostFtdcOrderField field;
		field._field = *pOrder;
		field.RecalculateHash();
		ftdcOrderFields.push_back(field);


		std::vector<MyCThostFtdcOrderField>::iterator it;
		bool find = false;
		for (it = pengdingFtdcOrderFields.begin();
			it != pengdingFtdcOrderFields.end(); ++it)
		{
			if (strcmp(it->_field.OrderRef, pOrder->OrderRef)==0
				&& it->_field.FrontID == pOrder->FrontID
				&& it->_field.SessionID == pOrder->SessionID)
			{
				find = true;
				if (pOrder->OrderSubmitStatus == THOST_FTDC_OSS_Accepted
					&& ((pOrder->VolumeTotal == 0
						&& pOrder->OrderStatus == THOST_FTDC_OST_AllTraded)
						|| pOrder->OrderStatus == THOST_FTDC_OST_Canceled))
				{
					pengdingFtdcOrderFields.erase(it);
					return;
				}
			}
		}

		if (!find
			&& pOrder->OrderSubmitStatus == THOST_FTDC_OSS_Accepted
			&& pOrder->VolumeTotal >0
			&& (pOrder->OrderStatus != THOST_FTDC_OST_PartTradedQueueing
				|| pOrder->OrderStatus != THOST_FTDC_OST_NoTradeQueueing))
		{
			pengdingFtdcOrderFields.push_back(field);
		}
	}*/

	//清除所有的报单
	void ClearCThostFtdcOrderField()
	{
		ftdcOrderFields.clear();
	}

	//插入成交相应
	void InsertCThostFtdcTradeField(CThostFtdcTradeField *pOrder)
	{
		
		ftdcTradeFields.push_back(*pOrder);

		// 删掉 成交的挂单
		//for (int i = 0; i < ftdcOrderFields.size(); ++i)
		std::vector<MyCThostFtdcOrderField>::iterator it;
		for( it = ftdcOrderFields.begin(); it != ftdcOrderFields.end(); ++it)
		{
			if (strcmp(it->_field.OrderRef,pOrder->OrderRef)==0
				&& strcmp(it->_field.OrderSysID , pOrder->OrderSysID)==0
				&& it->_field.Direction == pOrder->Direction
				&& it->_field.CombOffsetFlag[0] == pOrder->OffsetFlag)
			{
				ftdcOrderFields.erase(it);
				return;
			}
		}

		// 清理pending中所有已经成交的
		for (it = pengdingFtdcOrderFields.begin();
			it != pengdingFtdcOrderFields.end(); ++it)
		{
			if (strcmp(it->_field.OrderRef, pOrder->OrderRef) == 0
				&& strcmp(it->_field.OrderSysID, pOrder->OrderSysID) == 0
				&& it->_field.Direction == pOrder->Direction
				&& it->_field.CombOffsetFlag[0] == pOrder->OffsetFlag)
			{
				if (it->_field.OrderSubmitStatus == THOST_FTDC_OSS_Accepted
					&& ((it->_field.VolumeTotal == 0
						&& it->_field.OrderStatus == THOST_FTDC_OST_AllTraded)
						|| it->_field.OrderStatus == THOST_FTDC_OST_Canceled))
				{
					pengdingFtdcOrderFields.erase(it);
				}
			}
		}
		
	}

	//清除所有的成交相应记录
	void ClearCThostFtdcTradeField()
	{
		ftdcTradeFields.clear();
	}

	// 插入报单错误
	void InsertCThostFtdcInputOrderField(CThostFtdcInputOrderField *pField)
	{
		ftdcInputOrderFields.push_back(*pField);
	}

	//清除所有的报单错误
	void ClearCThostFtdcInputOrderField()
	{
		ftdcInputOrderFields.clear();
	}

	// 处理撤单响应,撤单成功
	void HandleRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		std::vector<MyCThostFtdcOrderField>::iterator it;
		for (it = pengdingFtdcOrderFields.begin(); it != pengdingFtdcOrderFields.end(); ++it)
		{
			if (strcmp(it->_field.OrderRef , pInputOrderAction->OrderRef)==0
				&& it->_field.FrontID == pInputOrderAction->FrontID
				&& it->_field.SessionID== pInputOrderAction->SessionID)
			{
				// 如果是撤单，那么删掉挂单，否则修改
				if (pInputOrderAction->ActionFlag == THOST_FTDC_AF_Delete)
				{
					pengdingFtdcOrderFields.erase(it);
				}
				else
				{

				}
				return;
			}
		}
	}

	//得到今天开仓的数量
	int GetTodayNumberOfInvestorPositions(TThostFtdcPosiDirectionType _posidirection)
	{
		if (investorPositionAllDirections.find(_posidirection) == investorPositionAllDirections.end()) return 0;

		int l_Counter = 0;

		for (size_t i = 0; i < investorPositionAllDirections[_posidirection].size(); ++i)
		{
			if (investorPositionAllDirections[_posidirection][i].PositionDate != THOST_FTDC_PSD_History)
			{
				l_Counter += investorPositionAllDirections[_posidirection][i].OpenVolume 
					+ investorPositionAllDirections[_posidirection][i].YdPosition 
					- investorPositionAllDirections[_posidirection][i].CloseVolume;
			}
		}

		return l_Counter;
	}

	//得到所有开仓的数量
	int GetTotalNumberOfInvestorPositions(TThostFtdcPosiDirectionType _posidirection)
	{
		if (investorPositionAllDirections.find(_posidirection) == investorPositionAllDirections.end()) return 0;

		int l_Counter = 0;

		for (size_t i = 0; i < investorPositionAllDirections[_posidirection].size(); ++i)
		{
				l_Counter += investorPositionAllDirections[_posidirection][i].OpenVolume
					+ investorPositionAllDirections[_posidirection][i].YdPosition
					- investorPositionAllDirections[_posidirection][i].CloseVolume;
		}

		return l_Counter;
	}

};

#endif