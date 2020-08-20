#ifndef STRATEGY_REDIS_WORKER_H__
#define STRATEGY_REDIS_WORKER_H__
#include "redisclient.h"
#include <map>
#include <vector>
//#include "ThostFtdcTraderApi.h"
#include "InstrumentRuntime.h"
class strategy_redis_worker
{
public:
	strategy_redis_worker(RedisClient &_redis);
	~strategy_redis_worker();

private:
	RedisClient &redis;
public:
	
	/*µÃµ½*/
	void get_ctp_instrument_value_simple(const char* instrumentid, const boost::function<void(const RedisValue &)> &handler);

	void publish_command(const char * channel,const char *command)
	{
		redis.publish(channel, command);
	}

	void refresh_redis_content_ctp_strategy();

	void get_local_ctp_strategies(const boost::function<void(const RedisValue &)> &handler);

	void refresh_single_strategy_values(const char * filename, int buy, int sell);

	void update_subscribe_instrument_ids(const char *investor);
};

#endif // !CTP_REDIS_WORKER_H__