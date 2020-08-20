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

	//��
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
	_cycle: int, 1,5,10,15,30 ������
	_code: ctp��instrumentid
	*/
	KLineGenerator(int _cycle, const char *_code);
	~KLineGenerator();

	//һ���ӵı�������ʱ����֧��3����,1 , 5  10 15 30 60(Сʱ)�����߲���Ҫ���㣬�������Ͽ���ͨ�����߼��㣬���ڴ���
	int cycle;
	std::string item_code;

	
	std::list<HistoryItem> buffer_history_items;

	HistoryItem current_item;

	const item_code_class &item_code_object;

};

#endif // !_KLINE_GENERATOR_H__