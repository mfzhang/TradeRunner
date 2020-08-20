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
		return _type == THOST_FTDC_PD_Long ? "��ͷ" : "��ͷ";
	}

	static std::string OpenDirectionDescription(CTP_COMMAND_ARGU_DIRECTION _direction)
	{
		return _direction == 1 ? "��" : "��";
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
	
	//ȡ���ҵ�������в���������true�����򷵻�false
	static bool CancelFtdcOrder(Trade &_trade, const char* ins, CTP_COMMAND_ARGU_DIRECTION &_dir, bool _open = false );
private:
	/*******************************����ִ��*******************************/
	// close all
	// �����߼��֣���ʵӦ��ֻ�ǿ�ƽ�־͹��ˣ�ֹӯֹ��������Ľ����в���
	// ��ǰ�׶Σ�����ֹӯֹ��Ĳ����������Ķ�ʵ����
	void execute_command_close_all();

	// ֻ����֤ ��ƽ��ָ��ͳɹ���û�취���������Ϊ����ƽ1�ֺ󣬻���1�֣�û�취��ȷ�����
	// here should be a reference to _currentCmd
	void execute_command_close(ctp_command &_cmd);

	// ֻ����֤ ��ƽ��ָ��ͳɹ���û�취���������Ϊ����ƽ1�ֺ󣬻���1�֣�û�취��ȷ�����
	// here should be a reference to _currentCmd
	void execute_command_lose_limit(ctp_command &_cmd);

	// ֻ����֤ ��ƽ��ָ��ͳɹ���û�취���������Ϊ����ƽ1�ֺ󣬻���1�֣�û�취��ȷ�����
	// here should be a reference to _currentCmd
	void execute_command_win_limit(ctp_command &_cmd);

	// ֻ����֤ ��ƽ��ָ��ͳɹ���û�취�����
	// here should be a reference to _currentCmd
	void execute_command_open(ctp_command &_cmd);

	void execute_command_cancelorder(ctp_command &_cmd);
	// �鿴������ʹ��
	void execute_list_all_cmd();

	// ˢ�³ֲ֣�����ʹ��
	void execute_refresh_ins_cmd(ctp_command &_cmd);

	void go_to_next_command();
	void execute_command_ensure(ctp_command &_cmd);
	/*******************************����ִ��*******************************/

	// ����ensure ����
	void buffer_ensure_ins_cmd(const ctp_command &_command);
	
	boost::mutex _buffer_mutex;
	std::vector<ctp_command> _cmdBuffers;
	ctp_command* _currentCmd;
	Trade &_trade;

	// TODO:�������Ҳ�ĳ�timer��ʱִ�аɡ������ܻ���ȱ��
	boost::asio::deadline_timer actionTimer;

	void handle_action_timer(const boost::system::error_code& error);
};

#endif // !CTP_COMMAND_HANLDER_H__