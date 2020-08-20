#define ELPP_THREAD_SAFE  
#include "InstrumentRuntime.h"
#include "my_utils.h"
#include <sstream>
#include "easylogging++.h"
InstrumentRuntime::InstrumentRuntime()
{
	latestValue = { 0 };
}


InstrumentRuntime::~InstrumentRuntime()
{
}

void MyCThostFtdcOrderField::RecalculateHash()
{
	std::stringstream ss;
	ss << _field.SessionID << _field.FrontID << _field.OrderRef;

	_hashCode = my_utils::hash(ss.str().c_str());
}

//插入报单,当前只存储pengding的
void InstrumentRuntime::InsertCThostFtdcOrderField(CThostFtdcOrderField *pOrder)
{
	MyCThostFtdcOrderField field;
	field._field = *pOrder;
	field.RecalculateHash();
	ftdcOrderFields.push_back(field);


	std::vector<MyCThostFtdcOrderField>::iterator it;
	bool find = false;
	for (it = pengdingFtdcOrderFields.begin();
		it != pengdingFtdcOrderFields.end(); ++it)
	{
		if (strcmp(it->_field.OrderRef, pOrder->OrderRef) == 0
			&& it->_field.FrontID == pOrder->FrontID
			&& it->_field.SessionID == pOrder->SessionID)
		{
			find = true;
			if (pOrder->OrderSubmitStatus == THOST_FTDC_OSS_Accepted
				&& ((pOrder->VolumeTotal == 0
					&& pOrder->OrderStatus == THOST_FTDC_OST_AllTraded)
					|| pOrder->OrderStatus == THOST_FTDC_OST_Canceled))
			{
				LOG(INFO) << "canceled order are removed from pending list";
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
}