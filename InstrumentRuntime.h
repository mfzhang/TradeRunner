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
	//��������ĳֲ�
	
	TwoDirectionsCThostFtdcInvestorPositionFieldMap investorPositionAllDirections;
	typedef const TwoDirectionsCThostFtdcInvestorPositionFieldMap::value_type ConstPosiDirectionTypes;

	//������Ӧ,
	std::vector<MyCThostFtdcOrderField> ftdcOrderFields;
	//std::map<unsigned int,CThostFtdcOrderField> ftdcOrderFields;
	// pending����,���ǹҵ�,�ĳ�map�ɣ�key��hash
	std::vector<MyCThostFtdcOrderField> pengdingFtdcOrderFields;
	//std::map<unsigned int, CThostFtdcOrderField> pengdingFtdcOrderFields;

	// �ɽ���¼
	std::vector<CThostFtdcTradeField> ftdcTradeFields;

	// ��������
	std::vector<CThostFtdcInputOrderField> ftdcInputOrderFields;


	InstrumentRuntime();
	~InstrumentRuntime();

	// ����ֲ�
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

	//����ֲ�
	void ClearCThostFtdcInvestorPositionField()
	{
		investorPositionAllDirections.clear();
	}

	//���뱨��,��ǰֻ�洢pengding��
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

	//������еı���
	void ClearCThostFtdcOrderField()
	{
		ftdcOrderFields.clear();
	}

	//����ɽ���Ӧ
	void InsertCThostFtdcTradeField(CThostFtdcTradeField *pOrder)
	{
		
		ftdcTradeFields.push_back(*pOrder);

		// ɾ�� �ɽ��Ĺҵ�
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

		// ����pending�������Ѿ��ɽ���
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

	//������еĳɽ���Ӧ��¼
	void ClearCThostFtdcTradeField()
	{
		ftdcTradeFields.clear();
	}

	// ���뱨������
	void InsertCThostFtdcInputOrderField(CThostFtdcInputOrderField *pField)
	{
		ftdcInputOrderFields.push_back(*pField);
	}

	//������еı�������
	void ClearCThostFtdcInputOrderField()
	{
		ftdcInputOrderFields.clear();
	}

	// ��������Ӧ,�����ɹ�
	void HandleRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		std::vector<MyCThostFtdcOrderField>::iterator it;
		for (it = pengdingFtdcOrderFields.begin(); it != pengdingFtdcOrderFields.end(); ++it)
		{
			if (strcmp(it->_field.OrderRef , pInputOrderAction->OrderRef)==0
				&& it->_field.FrontID == pInputOrderAction->FrontID
				&& it->_field.SessionID== pInputOrderAction->SessionID)
			{
				// ����ǳ�������ôɾ���ҵ��������޸�
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

	//�õ����쿪�ֵ�����
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

	//�õ����п��ֵ�����
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