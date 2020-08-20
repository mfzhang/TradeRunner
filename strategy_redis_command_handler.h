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
	// �����¼���������仯ʱ���ã�Ȼ�����get����
	void ctp_instrument_simple_value_update_handler(const std::vector<char> &buf);

	// ����仯ʱ��������ķ����������get�����Ļص��������õ�������ֵ
	void ctp_instrument_simple_value_result_handler(const RedisValue &buf);

	// ����redis�д洢�Ľ��ײ���
	void event_strategy_updated_handler(const std::vector<char> &buf);

	// ����仯ʱ��������ķ����������get�����Ļص��������õ�������ֵ
	void ctp_local_strategies_result_handler(const RedisValue &buf);
};
#endif // !STRATEGY_REDIS_COMMAND_HANDLER_H__