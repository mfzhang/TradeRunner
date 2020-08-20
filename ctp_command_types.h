#ifndef CTP_COMMAND_TYPES_H__
#define CTP_COMMAND_TYPES_H__
#include<string>
#include <vector>
typedef enum CTP_COMMAND_TYPE
{
	CTP_COMMAND_NULL = 0,
	CTP_COMMAND_CLOSE_ALL,
	CTP_COMMAND_CLOSE,
	CTP_COMMAND_OPEN,
	CTP_COMMAND_LIST_INVESTOR_POSITIONS,//上面的都是一次性的任务，下面的有些持久化的任务(就叫保障性任务吧)
	CTP_COMMAND_ENSURE_INS_DIR_NUMBERS,//该任务保障某个合约在某个方向开单数量，多了就平，少了就开（开平单可以加条件，暂时是无条件成功）
	CTP_COMMAND_LOSS_LIMIT,//止损，可以某天或者很多天
	CTP_COMMAND_WIN_LIMIT,//止盈
	CTP_COMMAND_RERESH_INS,
	CTP_COMMAND_CANCEL_ORDER,
}CTP_COMMAND_TYPE;


typedef enum CTP_COMMAND_ARGU_DIRECTION
{
	BUY_DIRECTION=1,
	SELL_DIRECTION=-1
}CTP_COMMAND_ARGU_DIRECTION;

class ctp_command_argus
{
public:
	ctp_command_argus():_instrumentId(""), _direction(CTP_COMMAND_ARGU_DIRECTION::SELL_DIRECTION),_price(0.0),_number(0), _enabled(true)
	{
	}
public:
	// 合约
	std::string _instrumentId;
	// 方向,这个地方是原始方向，表示多单还是空单
	CTP_COMMAND_ARGU_DIRECTION _direction;
	// 价格
	double _price;

	// 数量
	int _number;

	//
	bool _enabled;

	//cancel 
	int _sessionID;
	int _frontID;
	std::string _orderRef;
	// hash
	std::vector<unsigned int> _order_hash;
};

class ctp_command {
public:
	ctp_command():_type(CTP_COMMAND_TYPE::CTP_COMMAND_NULL){}
public:
	// command
	CTP_COMMAND_TYPE _type;

	// all argus
	std::vector<ctp_command_argus> _allArgus;

	bool empty() { return _allArgus.empty(); }
};
#endif // !CTP_COMMAND_TYPES_H__

