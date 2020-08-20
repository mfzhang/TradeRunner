#ifndef HISTORY_ITEM_H__
#define HISTORY_ITEM_H__
#include <map>
#include <vector>
#include <boost/date_time/posix_time/ptime.hpp>
#include <string>
typedef enum _FUTURE_INDEX_TYPE {
	MA = 0,
	MACD,
	KDJ,
	//.....,我应该只用ma就够了
} FUTURE_INDEX_TYPE;

struct MACDKey {
	int key1, key2, key3;
};

struct MACDValue {
	int value1, value2;
};
class HistoryItem
{
public:

	int tick_counter = 0;
	// pinzhong
	string m_Item;
	// start time
	boost::posix_time::ptime m_StartTime;
	// 
	boost::posix_time::ptime m_EndTime;
	//1,3,5,10,15,30,60,1day
	int m_nMinuteDual;

	string m_InstrumentCode;
	float m_OpenPrice;
	float m_HighPrice;
	float m_LowPrice;
	float m_ClosePrice;
	float m_nVolume;
	float m_StartVolume;
	HistoryItem(void);
	HistoryItem(const HistoryItem &_item)
	{
		m_Item = _item.m_Item;
		// start time
		m_StartTime = _item.m_StartTime;
		// 
		m_EndTime = _item.m_EndTime;
		//1,3,5,10,15,30,60,1day
		m_nMinuteDual = _item.m_nMinuteDual;

		m_InstrumentCode = _item.m_InstrumentCode;
		m_OpenPrice = _item.m_OpenPrice;
		m_HighPrice = _item.m_HighPrice;
		m_LowPrice = _item.m_LowPrice;
		m_ClosePrice = _item.m_ClosePrice;
		m_nVolume = _item.m_nVolume;

		m_Direction = _item.m_Direction;

		ma_values.clear();
		ma_values.insert(_item.ma_values.begin(), _item.ma_values.end());
	};
	~HistoryItem(void);

	int m_Direction = 0;
	
	//std::map<FUTURE_INDEX_TYPE, std::vector<float> > index_type_values;
	std::map<int, float> ma_values;
	std::map<MACDKey,MACDValue> macd_values;

public:
	void UpdatePrices(float _lastPrice)
	{
		m_ClosePrice = _lastPrice;
		if(m_HighPrice<m_ClosePrice)m_HighPrice = m_ClosePrice;
		
		if(m_LowPrice> m_ClosePrice || m_LowPrice <=0)m_LowPrice = m_ClosePrice;

		if (m_OpenPrice > _lastPrice)m_Direction = 1;
		else if (m_OpenPrice < _lastPrice)m_Direction = -1;
		// else equal, keep it as it is
	}

	void UpdatePrices(float _open, float _high, float _low, float _lastPrice)
	{
		m_OpenPrice = _open;
		m_HighPrice = _high;
		m_LowPrice = _low;
		m_ClosePrice = _lastPrice;

		if (m_OpenPrice > _lastPrice)m_Direction = 1;
		else if (m_OpenPrice < _lastPrice)m_Direction = -1;
		// else equal, keep it as it is
	}

	HistoryItem& operator=(HistoryItem& _item)
	{
		if (this == &_item)return*this;

		m_Item = _item.m_Item;
		m_StartTime = _item.m_StartTime;
		// 
		m_EndTime = _item.m_EndTime;
		//1,3,5,10,15,30,60,1day
		m_nMinuteDual = _item.m_nMinuteDual;

		m_InstrumentCode = _item.m_InstrumentCode;
		m_OpenPrice = _item.m_OpenPrice;
		m_HighPrice = _item.m_HighPrice;
		m_LowPrice = _item.m_LowPrice;
		m_ClosePrice = _item.m_ClosePrice;
		m_nVolume = _item.m_nVolume;

		m_Direction = _item.m_Direction;

		ma_values.clear();
		ma_values.insert(_item.ma_values.begin(), _item.ma_values.end());

		return *this;
	}
};

#endif