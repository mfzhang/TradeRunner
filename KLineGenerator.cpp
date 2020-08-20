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
	// ���ļ�����
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

	//���ʱ����У���ˣ���Ϊ��ǰ�ľ��Ƕ�Ӧ����
	//��ʱ����Ҫ�첽������Ϊֻ���������ʹ�������Լ��һ��Ʒ����ദ��������Լ
	// 1 5 10 15 30�� 60�۲�����ǣ���� ��11��30��1��30����֪��զ����
	//�õ�Сʱ ���� ��

	int hour, minute, second;
	//�°���е��ֶΣ����ƣ�������������
	//sscanf(pDepthMarketData->ActionDay, "%04d%02d%02d", &a, &b, &c);
	sscanf(pDepthMarketData->UpdateTime, "%2d:%02d:%02d", &hour, &minute, &second);

	
	boost::posix_time::ptime now1(boost::posix_time::second_clock::local_time());
	//�������ʱ�䣬�������ܴ����������
	boost::posix_time::ptime now(now1.date(), boost::posix_time::time_duration(hour, minute, 0));

	boost::posix_time::ptime pt1(now.date(), boost::posix_time::time_duration(hour, minute, 0));

	boost::posix_time::time_duration td = pt1 - current_item.m_StartTime;

	int minutes = td.hours() * 60 + td.minutes();

	//�����һ��
	// ����8��룬����8���
	if (current_item.m_ClosePrice <= 0.0f
		|| minutes<0) {//minutes<0��ζ�������յ�ҹ�̵����ݣ�ֱ�������µĸ���
		current_item.m_OpenPrice =
			current_item.m_HighPrice =
			current_item.m_LowPrice =
			current_item.m_ClosePrice = pDepthMarketData->LastPrice;

		//�м�ʱ����������Ϊ��0�뿪ʼ
		boost::posix_time::ptime pt1(now.date(), boost::posix_time::time_duration(hour, minute, 0));
		current_item.m_StartTime = pt1;
		current_item.m_EndTime = pt1;
		current_item.m_nVolume = pDepthMarketData->Volume;

		LOG(INFO) << boost::str(boost::format("%1% %2$02d:%3$02d:%4$02d") % to_iso_extended_string(current_item.m_StartTime.date()) % current_item.m_StartTime.time_of_day().hours() % current_item.m_StartTime.time_of_day().minutes() % current_item.m_StartTime.time_of_day().seconds());

		return;
	}
	bool maybeEnd = false;

	
	//��ǰʱ���29�֣�59�ֵ�59�룬��Ӧ�Ű������̻���ҹ�����̣�û�г���30�ģ���ʱ��϶�Ҫд���ݿ��
	if (hour == now.time_of_day().hours()
		&& (minute == 29 || minute == 59)
		&& second == 59) {
		maybeEnd = true;

		LOG(INFO) << item_code << "___" << cycle << ":maybe end true 1";
	}
	else if (second == 59) {
		// 4��59�� 9��59�� 14��59�� 29��59��
		if ((minute+1) % cycle == 0)
		{
			maybeEnd = true;
			LOG(INFO) << item_code << "___" << cycle << ":maybe end true 2";
		}
	}


	{
		

		//����϶�minutes >0
		// minutes>=0 ���������յ�ҹ�̵�����
		if (minutes >= 0) {
			if ((now.time_of_day().minutes()) % cycle == 0) {
				//�û��ˣ�����ط���Ҫ��ctp�Ĵ���ת���ɱ�׼���룬���ܣ�����������������
				if ((minutes == cycle
					|| (minutes>0 && minutes%cycle==0 && hour==now.time_of_day().hours())
					)&& current_item.tick_counter>10) // current_item.tick_counter>10 ����15:00�Ĵ��ȥ
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
				else {//�������µ�ǰֵ

					//����ط��������else�߼�һ����
					++current_item.tick_counter;
					current_item.m_EndTime = pt1;
					current_item.m_ClosePrice = pDepthMarketData->LastPrice;
					if (current_item.m_ClosePrice > current_item.m_HighPrice)
						current_item.m_HighPrice = current_item.m_ClosePrice;

					if (current_item.m_ClosePrice < current_item.m_LowPrice)
						current_item.m_LowPrice = current_item.m_ClosePrice;

					current_item.m_nVolume = pDepthMarketData->Volume - current_item.m_StartVolume;
					//29,59�ֵ�59�룬�����ܴ����Σ���ô���⣿
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
				//���û�д������̵���������鷳
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
				//29,59�ֵ�59�룬�����ܴ����Σ���ô���⣿
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
