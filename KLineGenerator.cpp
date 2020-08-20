#include "stdafx.h"
#include "redisclient.h"
#include "kline_redis_worker.h"
#include "KLineGenerator.h"
#include <json\json.h>
#include <fstream>
#include "easylogging++.h"
#include "ThostFtdcUserApiStruct.h"
//#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
//#include "HistorySqliteDB.h"
#include "my_utils.h"
#include <boost\format.hpp>
#include "ctp_message_def.h"
//#include "HistoryMysqlDb.h"
#include "HistoryItemDBWriter.h"
//


std::map<std::string, std::map<int, KLineGenerator*>> KLineGenerator::all_kline_generators;

extern kline_redis_worker k_redis_worker;
extern RedisClient publisher;
//extern HistoryMysqlDb history_db;
KLineGenerator::KLineGenerator(int _cycle, const char *_code):item_code_object(my_utils::get_item_code(_code))
{
	cycle = _cycle;
	item_code = item_code_object.name + item_code_object.date;
	current_item.m_nMinuteDual = cycle;

	current_item.m_InstrumentCode = _code;
	current_item.m_Item = item_code;
}
KLineGenerator::~KLineGenerator() {};

void KLineGenerator::Initilize(const char *filename) {
	// 打开文件保存
	ifstream l_file(filename);
	if (!l_file)
	{
		LOG(ERROR) << filename << " open failed";
		return;
	}

	std::string str((std::istreambuf_iterator<char>(l_file)),
		std::istreambuf_iterator<char>());
	l_file.close();

	Json::Reader parser;
	Json::Value root;
	if (parser.parse(str, root, false))
	{
		if (root.isArray())
		{
			for (Json::ArrayIndex i = 0; i < root.size(); ++i) {
				const char * code = root[i]["code"].asCString();
				Json::Value value = root[i]["cycle"];
				if (value.isArray()) {
					for (Json::ArrayIndex j = 0; j < value.size(); ++j) {
						int cycle = value[j].asInt();
						create(cycle, code);
					}
				}
			}
		}
	}
	
}

void KLineGenerator::handle(CThostFtdcDepthMarketDataField *pDepthMarketData)
{

	//这个时候不做校验了，认为当前的就是对应该类
	//暂时不需要异步处理，因为只处理主力和次主力合约，一个品种最多处理三个合约
	// 1 5 10 15 30； 60咱不处理，牵扯到 从11点30到1点30，不知道咋处理
	//得到小时 分钟 秒

	int hour, minute, second;
	//新版才有的字段，郁闷！！！！！！！
	//sscanf(pDepthMarketData->ActionDay, "%04d%02d%02d", &a, &b, &c);
	sscanf(pDepthMarketData->UpdateTime, "%2d:%02d:%02d", &hour, &minute, &second);

	
	boost::posix_time::ptime now1(boost::posix_time::second_clock::local_time());
	//用行情的时间，这样不能处理跨天的情况
	boost::posix_time::ptime now(now1.date(), boost::posix_time::time_duration(hour, minute, 0));

	boost::posix_time::ptime pt1(now.date(), boost::posix_time::time_duration(hour, minute, 0));

	boost::posix_time::time_duration td = pt1 - current_item.m_StartTime;

	int minutes = td.hours() * 60 + td.minutes();

	//当天第一个
	// 早上8点半，晚上8点半
	if (current_item.m_ClosePrice <= 0.0f
		|| minutes<0) {//minutes<0意味着早上收到夜盘的数据，直接用最新的覆盖
		current_item.m_OpenPrice =
			current_item.m_HighPrice =
			current_item.m_LowPrice =
			current_item.m_ClosePrice = pDepthMarketData->LastPrice;

		//中间时间启动，认为从0秒开始
		boost::posix_time::ptime pt1(now.date(), boost::posix_time::time_duration(hour, minute, 0));
		current_item.m_StartTime = pt1;
		current_item.m_EndTime = pt1;
		current_item.m_nVolume = pDepthMarketData->Volume;

		LOG(INFO) << boost::str(boost::format("%1% %2$02d:%3$02d:%4$02d") % to_iso_extended_string(current_item.m_StartTime.date()) % current_item.m_StartTime.time_of_day().hours() % current_item.m_StartTime.time_of_day().minutes() % current_item.m_StartTime.time_of_day().seconds());

		return;
	}
	bool maybeEnd = false;

	
	//当前时间的29分，59分的59秒，对应着白天收盘或者夜晚收盘，没有超过30的，这时候肯定要写数据库的
	if (hour == now.time_of_day().hours()
		&& (minute == 29 || minute == 59)
		&& second == 59) {
		maybeEnd = true;

		LOG(INFO) << item_code << "___" << cycle << ":maybe end true 1";
	}
	else if (second == 59) {
		// 4分59秒 9分59秒 14分59秒 29分59秒
		if ((minute+1) % cycle == 0)
		{
			maybeEnd = true;
			LOG(INFO) << item_code << "___" << cycle << ":maybe end true 2";
		}
	}


	{
		

		//这里肯定minutes >0
		// minutes>=0 忽略早上收到夜盘的数据
		if (minutes >= 0) {
			if ((now.time_of_day().minutes()) % cycle == 0) {
				//该换了，这个地方需要把ctp的代码转化成标准代码，性能！！！！！！！！！
				if ((minutes == cycle
					|| (minutes>0 && minutes%cycle==0 && hour==now.time_of_day().hours())
					)&& current_item.tick_counter>10) // current_item.tick_counter>10 避免15:00的存进去
				{
					//HistorySqliteDB &db = HistorySqliteDB::instancefast(item_code_object.name);
					/*HistoryMysqlDb &db = history_db;
					if (db.HistoryItemExist(current_item)) {
						db.UpdateHistoryItem(current_item);
					}
					else {
						db.SaveHistoryItem(current_item);
					}*/


					HistoryItemDBWriter::StoreHistoryItem(current_item);

					k_redis_worker.update_redis_kline_value(current_item);
					std::string value = boost::str(boost::format("%1%%2%%3%") %current_item.m_Item %ctp_message_def::KEY_KLINE_REDIS_VALUE %current_item.m_nMinuteDual);
					publisher.publish(ctp_message_def::EVENT_KLINE_VALUE_DONE,value);
				}

				if (minutes != 0)
				{
					LOG(INFO) << boost::str(boost::format("m_StartTime %1% %2$02d:%3$02d:%4$02d") % to_iso_extended_string(current_item.m_StartTime.date()) % current_item.m_StartTime.time_of_day().hours() % current_item.m_StartTime.time_of_day().minutes() % current_item.m_StartTime.time_of_day().seconds());
					LOG(INFO) << item_code << "___" << cycle << ": start of next cycle-------" << minutes;
					boost::posix_time::ptime pt2(now.date(), boost::posix_time::time_duration(hour, minute, 0));
					LOG(INFO) << boost::str(boost::format("pt2 %1% %2$02d:%3$02d:%4$02d") % to_iso_extended_string(pt2.date()) % pt2.time_of_day().hours() % pt2.time_of_day().minutes() % pt2.time_of_day().seconds());
					current_item.m_StartTime = pt2;
					current_item.m_EndTime = pt2;

					current_item.m_OpenPrice =
						current_item.m_HighPrice =
						current_item.m_LowPrice =
						current_item.m_ClosePrice = pDepthMarketData->LastPrice;

					current_item.m_StartVolume = pDepthMarketData->Volume;

					current_item.tick_counter = 1;
				}
				else {//持续更新当前值

					//这个地方跟下面的else逻辑一样，
					++current_item.tick_counter;
					current_item.m_EndTime = pt1;
					current_item.m_ClosePrice = pDepthMarketData->LastPrice;
					if (current_item.m_ClosePrice > current_item.m_HighPrice)
						current_item.m_HighPrice = current_item.m_ClosePrice;

					if (current_item.m_ClosePrice < current_item.m_LowPrice)
						current_item.m_LowPrice = current_item.m_ClosePrice;

					current_item.m_nVolume = pDepthMarketData->Volume - current_item.m_StartVolume;
					//29,59分的59秒，最后可能存两次，怎么避免？
					if (maybeEnd && minutes == cycle - 1) {
						//HistorySqliteDB &db = HistorySqliteDB::instancefast(item_code_object.name);
						/*HistoryMysqlDb &db = history_db;
						if (db.HistoryItemExist(current_item)) {
							db.UpdateHistoryItem(current_item);
						}
						else {
							db.SaveHistoryItem(current_item);
						}*/

						HistoryItemDBWriter::StoreHistoryItem(current_item);

						k_redis_worker.update_redis_kline_value(current_item);
					}

				}
				//这个没有处理收盘等情况，好麻烦
			}
			else {
				++current_item.tick_counter;
				current_item.m_EndTime = pt1;
				current_item.m_ClosePrice = pDepthMarketData->LastPrice;
				if (current_item.m_ClosePrice > current_item.m_HighPrice)
					current_item.m_HighPrice = current_item.m_ClosePrice;

				if (current_item.m_ClosePrice < current_item.m_LowPrice)
					current_item.m_LowPrice = current_item.m_ClosePrice;

				current_item.m_nVolume = pDepthMarketData->Volume - current_item.m_StartVolume;
				//29,59分的59秒，最后可能存两次，怎么避免？
				if (maybeEnd && minutes == cycle - 1) {
					//HistorySqliteDB &db = HistorySqliteDB::instancefast(item_code_object.name);
					/*HistoryMysqlDb &db = history_db;
					if (db.HistoryItemExist(current_item)) {
						db.UpdateHistoryItem(current_item);
					}
					else {
						db.SaveHistoryItem(current_item);
					}*/

					HistoryItemDBWriter::StoreHistoryItem(current_item);

					k_redis_worker.update_redis_kline_value(current_item);
				}
			}
		}

	}
}
