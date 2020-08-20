#include "ctp_simple_command_parser.h"
#include <boost/tokenizer.hpp>  
#include <boost/algorithm/string.hpp>  
#include <sstream>

// 平掉所有
const char * ctp_simple_command_parser::K_CTP_CMD_CLOSE_ALL_TAG = "close_all";
// 平掉某一个，或者某一些
const char * ctp_simple_command_parser::K_CTP_CMD_CLOSE_TAG = "close";
// 开仓
const char * ctp_simple_command_parser::K_CTP_CMD_OPEN_TAG = "open";

// 打印开仓 
const char * ctp_simple_command_parser::K_CTP_CMD_LIST_ALL_INVESTORS_TAG = "list_inv";

// 保证开单数量
const char * ctp_simple_command_parser::K_CTP_CMD_ENSURE_INS_DIR_TAG = "ensure_ins";

// 测试使用，刷新持仓
const char * ctp_simple_command_parser::K_CTP_CMD_REFRESH_INS_TAG="refresh_ins";

//取消挂单
const char * ctp_simple_command_parser::K_CTP_CMD_CANCEL_ORDER_TAG = "cancelorder";

ctp_simple_command_parser::ctp_simple_command_parser()
{
}

ctp_simple_command_parser::~ctp_simple_command_parser()
{
}

bool ctp_simple_command_parser::parse(std::string content, ctp_command &cmd)
{
	std::vector<std::string> _inputs;
	boost::split(_inputs, content, boost::is_any_of("|"), boost::token_compress_on);
	if (_inputs.empty()) return false;
	if (_inputs[0].compare(ctp_simple_command_parser::K_CTP_CMD_CLOSE_ALL_TAG) ==0 )
	{
		cmd._type = CTP_COMMAND_TYPE::CTP_COMMAND_CLOSE_ALL;
	}
	else if (_inputs[0].compare(ctp_simple_command_parser::K_CTP_CMD_CLOSE_TAG) ==0)
	{
		cmd._type = CTP_COMMAND_TYPE::CTP_COMMAND_CLOSE;
		if (_inputs.size() == 1) return false;

		parse_close_argus(_inputs[1], cmd);
	}else if (_inputs[0].compare(ctp_simple_command_parser::K_CTP_CMD_OPEN_TAG) ==0)
	{
		cmd._type = CTP_COMMAND_TYPE::CTP_COMMAND_OPEN;
		if (_inputs.size() == 1) return false;

		parse_close_argus(_inputs[1], cmd);
	}else if (_inputs[0].compare(ctp_simple_command_parser::K_CTP_CMD_LIST_ALL_INVESTORS_TAG) == 0)
	{
		cmd._type = CTP_COMMAND_TYPE::CTP_COMMAND_LIST_INVESTOR_POSITIONS;
	}
	else if (_inputs[0].compare(ctp_simple_command_parser::K_CTP_CMD_ENSURE_INS_DIR_TAG) == 0)
	{
		cmd._type = CTP_COMMAND_TYPE::CTP_COMMAND_ENSURE_INS_DIR_NUMBERS;
		parse_ensure_ins_dir_argus(_inputs[1], cmd);
		//这个地方可以把参数按照合约分割一下
	}
	else if (_inputs[0].compare(ctp_simple_command_parser::K_CTP_CMD_REFRESH_INS_TAG) == 0)
	{
		cmd._type = CTP_COMMAND_TYPE::CTP_COMMAND_RERESH_INS;
		parse_refresh_ins_argus(_inputs[1], cmd);
	}
	else if (_inputs[0].compare(ctp_simple_command_parser::K_CTP_CMD_CANCEL_ORDER_TAG) == 0)
	{
		cmd._type = CTP_COMMAND_TYPE::CTP_COMMAND_CANCEL_ORDER;
		parse_cancel_order_argus(_inputs[1], cmd);
	}
	else {
		return false;
	}
	return true;
}

/**********************************************解包函数*****************************************************/
void ctp_simple_command_parser::parse_ensure_ins_dir_argus(std::string content, ctp_command &cmd)
{
	//这两个格式是一样的，以后不一样的时候再改
	parse_close_argus(content, cmd);
}

// 刷新命令解析
// sample: refresh_ins|rb1710;rb1705;rb1705 //测试使用，查询持仓
void ctp_simple_command_parser::parse_refresh_ins_argus(std::string content, ctp_command &cmd)
{
	std::vector<std::string> _inputs;
	boost::split(_inputs, content, boost::is_any_of(";"), boost::token_compress_on);
	if (_inputs.empty()) return;

	for (size_t i = 0; i < _inputs.size(); ++i)
	{
		ctp_command_argus argu;
		argu._instrumentId = _inputs[i];
		cmd._allArgus.push_back(argu);
	}
}
void ctp_simple_command_parser::parse_close_argus(std::string content, ctp_command &cmd)
{
	std::vector<std::string> _inputs;
	boost::split(_inputs, content, boost::is_any_of(";"), boost::token_compress_on);
	if (_inputs.empty()) return;

	for (size_t i = 0; i < _inputs.size(); ++i)
	{
		try
		{
			std::vector<std::string> _argus;
			boost::split(_argus, _inputs[i], boost::is_any_of(","), boost::token_compress_on);
			if (_argus.size() < 3) continue;//可以么有价格,也可以没有指定是否有效

			ctp_command_argus argu;
			argu._instrumentId = _argus[0];
			argu._direction = (CTP_COMMAND_ARGU_DIRECTION)atoi(_argus[1].c_str());
			argu._number = atoi(_argus[2].c_str());
			if (_argus.size() > 3 && !_argus[3].empty())
			{
				argu._price = atof(_argus[3].c_str());
			}
			if (_argus.size() > 4 && !_argus[4].empty())
			{
				argu._enabled = atoi(_argus[4].c_str()) !=0?true:false;
			}
			cmd._allArgus.push_back(argu);
		}
		catch (std::exception *e)
		{

		}
	}
}

/**********************************************组包函数,静态********************************************/
std::string ctp_simple_command_parser::make_ensure_ins_directions_package(const char *instrumentid, int buy_number, int sell_number)
{
	std::stringstream ss;
	ss << K_CTP_CMD_ENSURE_INS_DIR_TAG << "|" 
		<< instrumentid << "," << 1 << "," << buy_number << ";" 
		<< instrumentid << "," << -1 << "," << sell_number;

	return ss.str();
}

void ctp_simple_command_parser::parse_cancel_order_argus(std::string content, ctp_command &cmd)
{
	std::vector<std::string> _inputs;
	boost::split(_inputs, content, boost::is_any_of(";"), boost::token_compress_on);
	if (_inputs.empty()) return;

	for (size_t i = 0; i < _inputs.size(); ++i)
	{
		try
		{
			std::vector<std::string> _argus;
			boost::split(_argus, _inputs[i], boost::is_any_of(","), boost::token_compress_on);
			if (_argus.size() < 1) continue;//必须四个

			ctp_command_argus argu;
			for (size_t j = 0; j < _argus.size();++j)
			{
				try {
					argu._order_hash.push_back( atoi((_argus[j].c_str())));
				}
				catch (std::exception *ee) {}
			}
			/*argu._instrumentId = _argus[0];
			argu._sessionID = atoi(_argus[1].c_str());
			argu._frontID = atoi(_argus[2].c_str());
			argu._orderRef = _argus[3].c_str();*/
			cmd._allArgus.push_back(argu);
		}
		catch (std::exception *e)
		{

		}
	}
}
