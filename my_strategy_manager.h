#ifndef MY_STRATEGY_MANAGER_H__
#define MY_STRATEGY_MANAGER_H__

#include "my_future_strategy.h"
#include <map>
#include <set>
#include <vector>
#include "InstrumentRuntime.h"
#include <boost/serialization/singleton.hpp>

/*
策略管理器，负责加载执行策略
*/
class strategy_redis_worker;
typedef std::map <std::string, std::map<std::string, std::vector<my_future_strategy>>> my_future_strategy_type;
class my_strategy_manager:public boost::serialization::singleton<my_strategy_manager>
{
public:
	my_strategy_manager();
	~my_strategy_manager();

	const my_future_strategy_type & get_all_strategies()
	{
		return _all_loaded_strategies;
	}
	// 从文件系统加载
	void load(const char * _parent_dir);
	

	// execute,执行完后，publish到redis
	void execute(std::string instrumentid, std::string datetime, InstrumentSimpleValue &value, strategy_redis_worker &redis_worker);

	// 得到所有的instrumentid，跟用户无关，暂不考虑性能
	std::set<std::string> get_all_scripted_instrumntids();

	// 得到所有的用户，跟品种无关，暂不考虑性能
	std::set<std::string> get_all_scripted_inverstors();

	// 添加策略
	void add_strategy_script(const char * _investor, const char* _instrumentid, const char *_script_file_name);

	// 该方法不更新本地文件系统
	void update_strategies_external(const my_future_strategy_type & external_strategies);

	std::string serializeStrategies();

	void set_current_running_investor(const char *inv)
	{
		running_investor = inv;
	}
private:
	// 从文件系统加载
	void load_investor_strategies(const char * _investor,const char * _parent_dir);

	// 从文件系统加载
	void load_instrument_strategies(const char * _investor, const char *_instrumentid,const char * _parent_dir);

public:
	void filter_by_investor();

	void start_strategy_running();
	void stop_strategy_running();
	bool threadrunning;
private:
	/*
	key1: investor, 用户登录名字
	key2: instrument id, 品种名字id
	包含了所有用户，所有品种的策略
	先不管别人，先执行自己的长线策略
	*/
	my_future_strategy_type _all_loaded_strategies;

	bool updating_external_strategies;

	std::string running_investor;
	
	
};

#endif