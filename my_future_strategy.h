#ifndef MY_FUTURE_STRATEGY_H__
#define MY_FUTURE_STRATEGY_H__
#include <string>

/*
策略类型，可以知道归属谁，哪个品种
每次策略执行，把实时行情传入
*/
class my_future_strategy
{
public:
	my_future_strategy();
	~my_future_strategy();
public:
	// owner of strategy
	std::string _investor;

	// instrument of strategy, can be used to subscribe instrument quote
	std::string _instrumenId;

	// lua script of strategy
	std::string _strategy_script;

	// finished？
	bool _finished;

	int _buy_count = -1;

	int _sell_count = -1;
};
#endif // !MY_FUTURE_STRATEGY_H__