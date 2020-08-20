#ifndef CTP_SIMPLE_COMMAND_PARSER_H__
#define CTP_SIMPLE_COMMAND_PARSER_H__

#include "ctp_command_types.h"
// sample: close_all
// sample: close|rb1701,1,10,0;rb1701,-1,10,2450;rm1701,1,10
// sample :open|rb1701,1,10;
// sample :ensure_ins|rb1701,1,10;rb1701,-1,10
// sample: refresh_ins|rb1710;rb1705;rb1705 //测试使用，查询持仓
// sample: cancel|rb1710,sessionid[int],frontid[int],orderref["1231231312"]
class ctp_simple_command_parser
{
public:
	ctp_simple_command_parser();
	~ctp_simple_command_parser();
	
	// 平掉所有
	static const char * K_CTP_CMD_CLOSE_ALL_TAG;
	// 平掉某一个，或者某一些
	static const char * K_CTP_CMD_CLOSE_TAG;
	// 开仓
	static const char * K_CTP_CMD_OPEN_TAG;

	//
	static const char * K_CTP_CMD_LIST_ALL_INVESTORS_TAG;

	// 保证开单数量
	static const char * K_CTP_CMD_ENSURE_INS_DIR_TAG;

	// 测试使用，刷新持仓
	static const char * K_CTP_CMD_REFRESH_INS_TAG;

	// 测试使用，刷新持仓
	static const char * K_CTP_CMD_CANCEL_ORDER_TAG;
public:
	// 解析
	bool parse(std::string content, ctp_command &cmd);

	// 组命令包,静态方法
	static std::string make_ensure_ins_directions_package(const char *instrumentid, int buy_number, int sell_number);
private:
	// 平仓命令解析
	void parse_close_argus(std::string content, ctp_command &cmd);
	//保证命令解析
	void parse_ensure_ins_dir_argus(std::string content, ctp_command &cmd);

	//刷新命令解析
	void parse_refresh_ins_argus(std::string content, ctp_command &cmd);

	//取消挂单命令
	void parse_cancel_order_argus(std::string content, ctp_command &cmd);
};

#endif // !CTP_SIMPLE_COMMAND_PARSER_H__