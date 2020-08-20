#ifndef CTP_REDIS_COMMAND_HANDLER_H__
#define CTP_REDIS_COMMAND_HANDLER_H__
#include "ctp_simple_command_parser.h"
#include "ctp_redis_worker.h"
#include <vector>
#include <map>
class ctp_command_handler;
class ctp_redis_command_handler
{
public:
	ctp_redis_command_handler(ctp_command_handler &handler,ctp_redis_worker &redis_worker, RedisClient &_cli);
	~ctp_redis_command_handler();
public:
	void execute(const std::vector<char> &buf);

	ctp_simple_command_parser _parser;
	ctp_command_handler &_handler;
	ctp_redis_worker &_redis_worker;
	RedisClient &subscriber;
	// 订阅事件，当行情变化时调用，然后调用get方法
	void ctp_instrument_simple_value_update_handler(const std::vector<char> &buf);

	// 行情变化时调用上面的方法，这个是get方法的回调，表明得到了行情值
	void ctp_instrument_simple_value_result_handler(const RedisValue &buf);

	//std::map<std::string, int> subscribedInstruments;
};
#endif // !CTP_REDIS_COMMAND_HANDLER_H__