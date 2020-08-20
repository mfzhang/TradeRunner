#ifndef _KLINE_GENERATOR_H__
#define _KLINE_GENERATOR_H__


#include "HistoryItem.h"

#include <string>
#include <map>
#include <list>
struct CThostFtdcDepthMarketDataField;
class item_code_class;
class KLineGenerator
{
public:
	static void Initilize(const char *filename);

	//根
	static std::map<int, KLineGenerator*> &get(const char * _code) {
		return all_kline_generators[_code];
	}

	void handle(CThostFtdcDepthMarketDataField *pDepthMarketData);
private:

	static void create(int _cycle, const char * _code)
	{
		if (all_kline_generators.find(_code) == all_kline_generators.end()
			|| all_kline_generators[_code].find(_cycle) == all_kline_generators[_code].end()) {

			
			KLineGenerator *generator = new KLineGenerator(_cycle, _code);
			

			if (generator != nullptr) {
				all_kline_generators[_code][_cycle] = generator;
				return;
			}
		}
	}
	static std::map<std::string, std::map<int, KLineGenerator*>> all_kline_generators;
	
	/*
	_cycle: int, 1,5,10,15,30 分钟数
	_code: ctp的instrumentid
	*/
	KLineGenerator(int _cycle, const char *_code);
	~KLineGenerator();

	//一分钟的倍数，暂时不想支持3分钟,1 , 5  10 15 30 60(小时)，日线不需要计算，日线以上可以通过日线计算，不在此列
	int cycle;
	std::string item_code;

	
	std::list<HistoryItem> buffer_history_items;

	HistoryItem current_item;

	const item_code_class &item_code_object;

};

#endif // !_KLINE_GENERATOR_H__