#ifndef MY_STRATEGY_MANAGER_H__
#define MY_STRATEGY_MANAGER_H__

#include "my_future_strategy.h"
#include <map>
#include <set>
#include <vector>
#include "InstrumentRuntime.h"
#include <boost/serialization/singleton.hpp>

/*
���Թ��������������ִ�в���
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
	// ���ļ�ϵͳ����
	void load(const char * _parent_dir);
	

	// execute,ִ�����publish��redis
	void execute(std::string instrumentid, std::string datetime, InstrumentSimpleValue &value, strategy_redis_worker &redis_worker);

	// �õ����е�instrumentid�����û��޹أ��ݲ���������
	std::set<std::string> get_all_scripted_instrumntids();

	// �õ����е��û�����Ʒ���޹أ��ݲ���������
	std::set<std::string> get_all_scripted_inverstors();

	// ��Ӳ���
	void add_strategy_script(const char * _investor, const char* _instrumentid, const char *_script_file_name);

	// �÷��������±����ļ�ϵͳ
	void update_strategies_external(const my_future_strategy_type & external_strategies);

	std::string serializeStrategies();

	void set_current_running_investor(const char *inv)
	{
		running_investor = inv;
	}
private:
	// ���ļ�ϵͳ����
	void load_investor_strategies(const char * _investor,const char * _parent_dir);

	// ���ļ�ϵͳ����
	void load_instrument_strategies(const char * _investor, const char *_instrumentid,const char * _parent_dir);

public:
	void filter_by_investor();

	void start_strategy_running();
	void stop_strategy_running();
	bool threadrunning;
private:
	/*
	key1: investor, �û���¼����
	key2: instrument id, Ʒ������id
	�����������û�������Ʒ�ֵĲ���
	�Ȳ��ܱ��ˣ���ִ���Լ��ĳ��߲���
	*/
	my_future_strategy_type _all_loaded_strategies;

	bool updating_external_strategies;

	std::string running_investor;
	
	
};

#endif