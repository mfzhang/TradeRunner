#include "my_strategy_manager.h"
#include "lua_helper.h"
#include "ctp_simple_command_parser.h"
#include "strategy_redis_worker.h"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <vector>
#include <string>
#include "my_utils.h"
#include "json/json.h"
#include <thread>
#define ELPP_THREAD_SAFE  
#include "easylogging++.h"
#include "strategy_context.h"
#include "strategy_redis_worker.h"
//#include "HistorySqliteDB.h"
#include "HistoryMysqlDb.h"

using namespace std;
extern strategy_redis_worker worker;

my_strategy_manager::my_strategy_manager()
{
	updating_external_strategies = false;
}


my_strategy_manager::~my_strategy_manager()
{
	threadrunning = false;
}

//加载
void my_strategy_manager::load(const char * _parent_dir)
{
	namespace fs = boost::filesystem;
	fs::path fullpath(_parent_dir, fs::native);

	if (!fs::exists(fullpath)) 
	{ 
		return; 
	}
	fs::directory_iterator end_iter;
	for (fs::directory_iterator iter(fullpath); iter != end_iter; iter++) 
	{
		// 不递归，第一层
		try 
		{
			if (fs::is_directory(*iter)) //这里应该是investor
			{
				load_investor_strategies(iter->path().filename().string().c_str(),iter->path().string().c_str());
			}
		}
		catch (const std::exception & ex) 
		{
			continue;
		}
	}
}

//加载
void my_strategy_manager::load_investor_strategies(const char * _investor, const char *_parent_dir)
{
	LOG(DEBUG) << "load_investor_strategies----Investor:" << _investor << ";Directory:" << _parent_dir;
	namespace fs = boost::filesystem;
	fs::path fullpath(_parent_dir, fs::native);

	if (!fs::exists(fullpath))
	{
		return;
	}
	fs::directory_iterator end_iter;
	for (fs::directory_iterator iter(fullpath); iter != end_iter; iter++)
	{
		try
		{
			if (fs::is_directory(*iter)) //这里应该是instrument
			{
				load_instrument_strategies(_investor, iter->path().filename().string().c_str(),iter->path().string().c_str());
			}
			else if(iter->path().string().compare(my_utils::K_MANIFEST_FILE_NAME) ==0)// 如果是manifest,记录了一些基本信息
			{
				// 暂时不处理
			}
		}
		catch (const std::exception & ex)
		{
			continue;
		}
	}
}

//加载
void my_strategy_manager::load_instrument_strategies(const char * _investor, const char *_instrumentid, const char *_parent_dir)
{
	LOG(DEBUG) << "load_instrument_strategies----Investor:" << _investor << ";InstrumentID:" << _instrumentid << ";Directory:" << _parent_dir;
	namespace fs = boost::filesystem;
	fs::path fullpath(_parent_dir, fs::native);

	if (!fs::exists(fullpath))
	{
		return;
	}
	fs::directory_iterator end_iter;
	for (fs::directory_iterator iter(fullpath); iter != end_iter; iter++)
	{
		try
		{
			if (fs::is_directory(*iter)) 
			{
				// do nothing	
			}
			else if (iter->path().string().compare(my_utils::K_MANIFEST_FILE_NAME) == 0)// 如果是manifest,记录了一些基本信息
			{
				// 暂时不处理
			}
			else if(iter->path().string().rfind(my_utils::K_STRATEGY_SCRIPT_EXTENSION) != string::npos)
			{
				add_strategy_script(_investor, _instrumentid, iter->path().string().c_str());
			}
		}
		catch (const std::exception & ex)
		{
			continue;
		}
	}
}

//直接添加
void my_strategy_manager::add_strategy_script(const char * _investor, const char* _instrumentid, const char *_script_file_name)
{
	LOG(DEBUG) << "add_strategy_script----Investor:" << _investor << ";InstrumentID:" << _instrumentid << ";Script file:" << _script_file_name;
	my_future_strategy mfs;
	mfs._finished = false;
	mfs._instrumenId = _instrumentid;
	mfs._investor = _investor;
	mfs._strategy_script = _script_file_name;
	_all_loaded_strategies[_investor][_instrumentid].push_back(mfs);
}
// 执行策略 
void my_strategy_manager::execute(std::string instrumentid, std::string datetime, InstrumentSimpleValue &value, strategy_redis_worker &redis_worker)
{
	if (updating_external_strategies) return;

	bool strategy_updated = false;
	// 第一层，investor
	std::map <std::string, std::map<std::string, std::vector<my_future_strategy>>>::iterator it;
	for (it = _all_loaded_strategies.begin(); it != _all_loaded_strategies.end(); ++it)
	{
		int l_ins_total_buy_cnt = 0;
		int l_ins_total_sell_cnt = 0;
		// 第二层 instrument id
		std::map<std::string, std::vector<my_future_strategy>>::iterator it1;
		for (it1 = it->second.begin(); it1 != it->second.end(); ++it1)
		{
			// 不是当前的合约
			if (it1->first.compare(instrumentid) != 0) continue;
			for (size_t i = 0; i < it1->second.size(); ++i)
			{
				int l_buy_number = 0;
				int l_sell_number = 0;

				my_future_strategy&l_strategy = it1->second[i];
				//if (it1->second[i]._finished) continue; //这个东西也要传入脚本
				// 真正执行，需要一个类负责执行lua
				lua_helper::invoke_script(l_strategy._strategy_script.c_str(),
					l_strategy._finished,
					value.open,
					value.high,
					value.low,
					value.close,
					datetime.c_str(),
					l_buy_number,
					l_sell_number);
				
				if (l_strategy._buy_count != l_buy_number
					|| l_strategy._sell_count != l_sell_number)
				{
					l_strategy._buy_count = l_buy_number;
					l_strategy._sell_count = l_sell_number;
					//strategy_updated = true;

					//这个时候单独更新某个策略
					redis_worker.refresh_single_strategy_values(l_strategy._strategy_script.c_str(), l_buy_number, l_sell_number);
				}
				// 找到.demo.strategy说明是测试的，不通知交易程序
				if (l_strategy._strategy_script.find(".demo.strategy") == std::string::npos)
				{
					l_ins_total_buy_cnt += l_buy_number;
					l_ins_total_sell_cnt += l_sell_number;
				}
			}

			// 这里得到了某个品种下所有的单子数量
			// 现在所有的策略执行，都不是即使开单性质的，都是保障多空数量
			// publish一个消息
			std::string redis_command = ctp_simple_command_parser::make_ensure_ins_directions_package(it1->first.c_str(), l_ins_total_buy_cnt, l_ins_total_sell_cnt);
			std::string channel = "investor_" + it->first;
			LOG(DEBUG) << "Channel:" << channel << ";Command:" << redis_command;
			redis_worker.publish_command(channel.c_str(),redis_command.c_str());
		}


		
	}

	if (strategy_updated)
	{
		redis_worker.refresh_redis_content_ctp_strategy();
	}
}

// 得到所有的合约
std::set<std::string> my_strategy_manager::get_all_scripted_instrumntids()
{
	std::set<std::string> ret;

	if (updating_external_strategies) return ret;

	std::map <std::string, std::map<std::string, std::vector<my_future_strategy>>>::iterator it;
	for (it = _all_loaded_strategies.begin(); it != _all_loaded_strategies.end(); ++it)
	{
		std::map<std::string, std::vector<my_future_strategy>>::iterator it1;
		for (it1 = it->second.begin(); it1 != it->second.end(); ++it1)
		{
			ret.insert(it1->first);
		}
	}

	return ret;
}

// 得到所有配置了脚本的投资者
std::set<std::string> my_strategy_manager::get_all_scripted_inverstors()
{
	
	std::set<std::string> ret;
	if (updating_external_strategies) return ret;

	std::map <std::string, std::map<std::string, std::vector<my_future_strategy>>>::iterator it;
	for (it = _all_loaded_strategies.begin(); it != _all_loaded_strategies.end(); ++it)
	{
		ret.insert(it->first);
	}

	return ret;
}

void my_strategy_manager::update_strategies_external(const my_future_strategy_type & external_strategies)
{
	updating_external_strategies = true;

	my_future_strategy_type::const_iterator it;
	for (it = external_strategies.cbegin(); it != external_strategies.cend(); ++it)
	{
		if (_all_loaded_strategies.find(it->first) == _all_loaded_strategies.end())
		{
			_all_loaded_strategies[it->first] = it->second;
		}
		else
		{
			std::map<std::string, std::vector<my_future_strategy>> &ins_strategies = _all_loaded_strategies[it->first];
			std::map<std::string, std::vector<my_future_strategy>>::const_iterator cit;
			for (cit = it->second.cbegin(); cit != it->second.cend(); ++cit)
			{
				if (ins_strategies.find(cit->first) == ins_strategies.end())
				{
					ins_strategies[cit->first] = cit->second;
				}
				else
				{
					std::vector<my_future_strategy> &strategies = ins_strategies[cit->first];
					for (size_t i = 0; i < cit->second.size(); ++i)
					{
						bool find = false;
						for (size_t j = 0; j < strategies.size(); ++j)
						{
							if (strategies[j]._strategy_script.compare(cit->second[j]._strategy_script) == 0)
							{
								strategies[j]._buy_count = cit->second[j]._buy_count;
								strategies[j]._sell_count = cit->second[j]._sell_count;
								strategies[j]._finished = cit->second[j]._finished;
								strategies[j]._instrumenId = cit->second[j]._instrumenId;
								strategies[j]._investor = cit->second[j]._investor;
								find = true;
								break;
							}
						}

						if (!find)
						{
							strategies.push_back(cit->second[i]);
						}
					}
				}
			}
		}
	}
	/*_all_loaded_strategies.clear();
	_all_loaded_strategies.insert(external_strategies.begin(), external_strategies.end());*/

	updating_external_strategies = false;
}

std::string my_strategy_manager::serializeStrategies()
{
	const my_future_strategy_type & all_strategies = this->_all_loaded_strategies;
	Json::Value root(Json::ValueType::arrayValue);
	// 第一层 investor
	my_future_strategy_type::const_iterator it;
	for (it = all_strategies.cbegin(); it != all_strategies.cend(); ++it)
	{
		Json::Value investor;
		investor["investor"] = it->first;
		Json::Value vec(Json::ValueType::arrayValue);

		// 第二层 instrument id
		std::map<std::string, std::vector<my_future_strategy>>::const_iterator it1;
		for (it1 = it->second.cbegin(); it1 != it->second.cend(); ++it1)
		{
			Json::Value ins;
			ins["instrumentid"] = it1->first;
			Json::Value insvec(Json::ValueType::arrayValue);
			for (size_t i = 0; i < it1->second.size(); ++i)
			{
				Json::Value val;
				val["script"] = it1->second[i]._strategy_script;
				val["finished"] = it1->second[i]._finished;
				val["buy"] = it1->second[i]._buy_count;
				val["sell"] = it1->second[i]._sell_count;
				insvec.append(val);
			}
			ins["strategies"] = insvec;
			vec.append(ins);
		}
		investor["instruments"] = vec;
		root.append(investor);
	}

	std::string strValue = root.toStyledString();
	return strValue;
}

void my_strategy_manager::filter_by_investor()
{
	if (running_investor.length() <= 0) return;
	std::set<std::string> deleted;
	my_future_strategy_type::iterator it;
	for (it = _all_loaded_strategies.begin(); it != _all_loaded_strategies.end(); ++it)
	{
		if (it->first.compare(running_investor) != 0)
			deleted.insert(it->first);
	}

	std::set<std::string>::iterator it1;
	for (it1 = deleted.begin(); it1 != deleted.end(); ++it1)
	{
		_all_loaded_strategies.erase(*it1);
	}
}

void strategy_run() 
{
	my_strategy_manager &instance= my_strategy_manager::get_mutable_instance();
	std::set<std::string> idsets = instance.get_all_scripted_instrumntids();
	strategy_context &context = strategy_context::get_mutable_instance();
	InstrumentSimpleValue insv;
	while (instance.threadrunning) {

		std::set<std::string>::iterator it;
		for (it = idsets.begin(); it != idsets.end(); ++it)
		{
			item_code_class ic = my_utils::get_item_code((*it).c_str());
			//如果
			/*if (HistorySqliteDB::instrument_item_history_k_line_receiving[ic.name+ic.date])*/
			if(strategy_context::get_mutable_instance().get_history_mysql_db().instrument_item_history_k_line_receiving[ic.name+ic.date])
				continue;
			if (context.get_instrument_simple_value((*it).c_str(), insv))
			{
				instance.execute(*it, insv.tradingDay, insv, worker);
			}
		}
		//std::this_thread::sleep_for(std::chrono::seconds(1));//1秒基本上够了
		std::this_thread::sleep_for(std::chrono::milliseconds(400));//1秒基本上够了
	}
}

void my_strategy_manager::start_strategy_running()
{
	threadrunning = true;
	std::thread t(strategy_run);
	t.detach();
}
void my_strategy_manager::stop_strategy_running()
{
	threadrunning = false;
}