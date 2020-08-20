#ifndef KLINE_REDIS_WORKER_H__
#define KLINE_REDIS_WORKER_H__
#include "redisclient.h"
#include <map>
#include <vector>

class HistoryItem;
class kline_redis_worker
{
public:
	kline_redis_worker(RedisClient &_redis);
	~kline_redis_worker();

private:
	RedisClient &redis;
public:

	void update_redis_kline_value(HistoryItem &_item);
};

#endif // !CTP_REDIS_WORKER_H__