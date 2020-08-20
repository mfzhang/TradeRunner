#ifndef CTP_COMMAND_HANLDER_H__
#define CTP_COMMAND_HANLDER_H__
#include "ctp_command_types.h"
#include "ThostFtdcUserApiDataType.h"
#include <boost/thread/mutex.hpp>
#include "redisclient/redisvalue.h"
#include <boost/asio.hpp>
class Trade;
class ctp_command_handler
{
public:
	ctp_command_handler(Trade &trade);
	~ctp_command_handler();

	void execute(const ctp_command &_command);
	void run();

	static int ConvertCmdArguDirectionToBuyDirector(CTP_COMMAND_ARGU_DIRECTION _type)
	{
		return _type == CTP_COMMAND_ARGU_DIRECTION::BUY_DIRECTION ? 0 : 1;
	}

	static int ConvertPosiDirectionToCloseDirector(TThostFtdcPosiDirectionType _type)
	{
		return _type == THOST_FTDC_PD_Long ? 1 : 0;
	}

	static std::string PosiDirectionDescription(TThostFtdcPosiDirectionType _type)
	{
		return _type == THOST_FTDC_PD_Long ? "多头" : "空头";
	}

	static std::string OpenDirectionDescription(CTP_COMMAND_ARGU_DIRECTION _direction)
	{
		return _direction == 1 ? "多" : "空";
	}
	static CTP_COMMAND_ARGU_DIRECTION ConvertPosiDirectionToCmdArguDirection(TThostFtdcPosiDirectionType _type)
	{
		return _type == THOST_FTDC_PD_Long ? CTP_COMMAND_ARGU_DIRECTION::BUY_DIRECTION : CTP_COMMAND_ARGU_DIRECTION::SELL_DIRECTION;
	}

	static TThostFtdcPosiDirectionType ConvertCmdArguDirectionToPosiDirection(CTP_COMMAND_ARGU_DIRECTION _type)
	{
		return _type ==  CTP_COMMAND_ARGU_DIRECTION::BUY_DIRECTION? THOST_FTDC_PD_Long  : THOST_FTDC_PD_Short;
	}

	static void CloseInvestorPositions(Trade &_trade,std::string _ins, CTP_COMMAND_ARGU_DIRECTION &_dir, int _number,float price=0.0f);
	
	//取消挂单，如果有操作，返回true，否则返回false
	static bool CancelFtdcOrder(Trade &_trade, const char* ins, CTP_COMMAND_ARGU_DIRECTION &_dir, bool _open = false );
private:
	/*******************************命令执行*******************************/
	// close all
	// 按照逻辑分，其实应该只是开平仓就够了，止盈止损都在另外的进程中操作
	// 当前阶段，除了止盈止损的不做，其他的都实现了
	void execute_command_close_all();

	// 只负责保证 开平仓指令发送成功，没办法检查结果，因为可能平1手后，还有1手，没办法精确检查结果
	// here should be a reference to _currentCmd
	void execute_command_close(ctp_command &_cmd);

	// 只负责保证 开平仓指令发送成功，没办法检查结果，因为可能平1手后，还有1手，没办法精确检查结果
	// here should be a reference to _currentCmd
	void execute_command_lose_limit(ctp_command &_cmd);

	// 只负责保证 开平仓指令发送成功，没办法检查结果，因为可能平1手后，还有1手，没办法精确检查结果
	// here should be a reference to _currentCmd
	void execute_command_win_limit(ctp_command &_cmd);

	// 只负责保证 开平仓指令发送成功，没办法检查结果
	// here should be a reference to _currentCmd
	void execute_command_open(ctp_command &_cmd);

	void execute_command_cancelorder(ctp_command &_cmd);
	// 查看，测试使用
	void execute_list_all_cmd();

	// 刷新持仓，测试使用
	void execute_refresh_ins_cmd(ctp_command &_cmd);

	void go_to_next_command();
	void execute_command_ensure(ctp_command &_cmd);
	/*******************************命令执行*******************************/

	// 缓存ensure 命令
	void buffer_ensure_ins_cmd(const ctp_command &_command);
	
	boost::mutex _buffer_mutex;
	std::vector<ctp_command> _cmdBuffers;
	ctp_command* _currentCmd;
	Trade &_trade;

	// TODO:这个东西也改成timer定时执行吧。否则总会有缺陷
	boost::asio::deadline_timer actionTimer;

	void handle_action_timer(const boost::system::error_code& error);
};

#endif // !CTP_COMMAND_HANLDER_H__