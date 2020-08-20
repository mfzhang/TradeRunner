#ifndef STRATEGY_REDIS_COMMAND_HANDLER_H__
#define STRATEGY_REDIS_COMMAND_HANDLER_H__
#include <vector>
class strategy_redis_worker;
class RedisValue;
class strategy_redis_command_handler
{
public:
	strategy_redis_command_handler(strategy_redis_worker &redis_worker);
	~strategy_redis_command_handler();
public:

	strategy_redis_worker &_redis_worker;
	// 订阅事件，当行情变化时调用，然后调用get方法
	void ctp_instrument_simple_value_update_handler(const std::vector<char> &buf);

	// 行情变化时调用上面的方法，这个是get方法的回调，表明得到了行情值
	void ctp_instrument_simple_value_result_handler(const RedisValue &buf);

	// 更新redis中存储的交易策略
	void event_strategy_updated_handler(const std::vector<char> &buf);

	// 行情变化时调用上面的方法，这个是get方法的回调，表明得到了行情值
	void ctp_local_strategies_result_handler(const RedisValue &buf);
};
#endif // !STRATEGY_REDIS_COMMAND_HANDLER_H__