#define ELPP_THREAD_SAFE  
#include "ctp_command_handler.h"
#include "TradeContext.h"
#include "trade.h"
#include "easylogging++.h"
#include <boost/thread/lock_guard.hpp>
#include "ctp_redis_worker.h"

ctp_command_handler::ctp_command_handler(Trade &trade):_trade(trade),actionTimer(trade.tradeio)
{
	_currentCmd = nullptr;
	actionTimer.expires_from_now(boost::posix_time::minutes(100));
	actionTimer.async_wait(boost::bind(&ctp_command_handler::handle_action_timer, this, boost::asio::placeholders::error));
}


ctp_command_handler::~ctp_command_handler()
{
}

void ctp_command_handler::handle_action_timer(const boost::system::error_code& error)
{
	static bool running = false;

	
	if (running)return;

	//����������������֮һ���ȴ�200ms
	if (!_trade.CanTradeExternal()) {
		actionTimer.expires_from_now(boost::posix_time::milliseconds(200));
		actionTimer.async_wait(boost::bind(&ctp_command_handler::handle_action_timer, this, boost::asio::placeholders::error));
		return;
	};

	running = true;
	run();

	{
		boost::lock_guard<boost::mutex> guard(_buffer_mutex);
		if (_cmdBuffers.empty())
		{
			//LOG(INFO) << "Long time waitting for command handler running";
			running = false;
			actionTimer.expires_from_now(boost::posix_time::minutes(100));
		}
		else
		{
			//LOG(INFO) << "Short time waitting for command handler running";
			running = false;
			actionTimer.expires_from_now(boost::posix_time::milliseconds(200));
		}
	}
	actionTimer.async_wait(boost::bind(&ctp_command_handler::handle_action_timer, this, boost::asio::placeholders::error));

}
void ctp_command_handler::execute(const ctp_command &_command)
{
	if (!_trade.CanTradeExternal())return;

	if(_command._type != CTP_COMMAND_TYPE::CTP_COMMAND_NULL)
	{
		
		switch (_command._type)
		{
		case CTP_COMMAND_TYPE::CTP_COMMAND_ENSURE_INS_DIR_NUMBERS://���û�еģ���ȥ��顣���������еĶ�ƽ��
			buffer_ensure_ins_cmd(_command);
			break;
		default:
			{
				boost::lock_guard<boost::mutex> guard(_buffer_mutex);
				_cmdBuffers.push_back(_command);
			}
			break;
		}

		// ���̳���
		actionTimer.cancel();
	}
	
}

void ctp_command_handler::buffer_ensure_ins_cmd(const ctp_command &_command)
{
	if (_command._type != CTP_COMMAND_TYPE::CTP_COMMAND_ENSURE_INS_DIR_NUMBERS)
	{
		execute(_command);
		return;
	}

	if (_command._allArgus.empty())  return;

	// ѭ���������еĲ��������ܰ��������Լ
	for (size_t _c_argu_idx = 0; _c_argu_idx < _command._allArgus.size(); ++_c_argu_idx)
	{
		size_t buf_idx;
		{
			boost::lock_guard<boost::mutex> guard(_buffer_mutex);
			for (buf_idx = 0; buf_idx < _cmdBuffers.size(); ++buf_idx)
			{
				if (_cmdBuffers[buf_idx]._allArgus.size() == 0) continue;
				// ��ǰ��Լ�ĸ�����
				if (_cmdBuffers[buf_idx]._type == _command._type
					&& _cmdBuffers[buf_idx]._allArgus[0]._instrumentId.compare(_command._allArgus[_c_argu_idx]._instrumentId) == 0)//û�е�Ӧ�ò���ӽ���
				{
					size_t argu_idx;
					for (argu_idx = 0; argu_idx < _cmdBuffers[buf_idx]._allArgus.size(); ++argu_idx)
					{
						if (_cmdBuffers[buf_idx]._allArgus[argu_idx]._direction == _command._allArgus[_c_argu_idx]._direction)//�ҵ�ĳ�������
						{
							_cmdBuffers[buf_idx]._allArgus[argu_idx] = _command._allArgus[_c_argu_idx];
							break;
						}
					}

					if (argu_idx == _cmdBuffers[buf_idx]._allArgus.size()
						&& _command._allArgus[_c_argu_idx]._enabled)//û���ҵ�,������enabled
					{
						_cmdBuffers[buf_idx]._allArgus.push_back(_command._allArgus[_c_argu_idx]);
					}

					// �ҵ���
					break;
				}
			}
			if (buf_idx == _cmdBuffers.size()
				&& _command._allArgus[_c_argu_idx]._enabled)//û�ҵ�
			{
				ctp_command _cmd;
				_cmd._type = _command._type;
				_cmd._allArgus.push_back(_command._allArgus[_c_argu_idx]);
				_cmdBuffers.push_back(_cmd);
			}
		}

		
	}
}

// �������Ҫ��Ҫ�ĳ�ѭ��ִ�У�
void ctp_command_handler::run()
{
	//LOG(DEBUG) <<"Entering function "<< __FUNCTION__;
	{
		boost::lock_guard<boost::mutex> guard(_buffer_mutex);
		// ��������ָ�����ָ�����в����Ļ�������ִ��
		while (!_cmdBuffers.empty() 
			&& (_cmdBuffers[0]._type == CTP_COMMAND_TYPE::CTP_COMMAND_NULL
				||_cmdBuffers[0].empty()))
		{
			_cmdBuffers.erase(_cmdBuffers.begin());
		}

		if (_cmdBuffers.empty()) return;

		_currentCmd = &_cmdBuffers[0];
	}

	/*if (_trade.m_bRequestInProgress)
	{
		LOG(DEBUG) << __FUNCTION__<<"Trade in progress, wait for next loop";
		return;
	}*/
	//��ʵ����һ��
	if (_currentCmd->_type == CTP_COMMAND_TYPE::CTP_COMMAND_CLOSE_ALL)
	{
		execute_command_close_all();
	}
	else if (_currentCmd->_type == CTP_COMMAND_TYPE::CTP_COMMAND_LIST_INVESTOR_POSITIONS)
	{
		execute_list_all_cmd();
	}
	else if (_currentCmd->_type == CTP_COMMAND_TYPE::CTP_COMMAND_CLOSE)
	{
		//ƽ��
		execute_command_close(*_currentCmd);
	}
	else if (_currentCmd->_type == CTP_COMMAND_TYPE::CTP_COMMAND_OPEN)
	{
		//����
		execute_command_open(*_currentCmd);
	}
	else if (_currentCmd->_type == CTP_COMMAND_TYPE::CTP_COMMAND_ENSURE_INS_DIR_NUMBERS)//
	{
		// ���ϵ���ֻ��һ�ұ��޷ֺţ��ʺ�׷��
		execute_command_ensure(*_currentCmd);
	}
	else if (_currentCmd->_type == CTP_COMMAND_TYPE::CTP_COMMAND_LOSS_LIMIT)
	{
		// ֹ��,��ʵ��,����ҵ�����
		//execute_command_lose_limit(*_currentCmd);
	}
	else if (_currentCmd->_type == CTP_COMMAND_TYPE::CTP_COMMAND_WIN_LIMIT)
	{
		// ֹӯ,��ʵ��,����ҵ�����
		//execute_command_win_limit(*_currentCmd);
	}
	else if (_currentCmd->_type == CTP_COMMAND_TYPE::CTP_COMMAND_RERESH_INS)
	{
		execute_refresh_ins_cmd(*_currentCmd);
	}
	else if (_currentCmd->_type == CTP_COMMAND_TYPE::CTP_COMMAND_CANCEL_ORDER)
	{
		//ƽ��
		execute_command_cancelorder(*_currentCmd);
	}

	//���е����궼����
	_currentCmd->_allArgus.clear();
	go_to_next_command();
	//LOG(DEBUG) << "Leave function " << __FUNCTION__;
}
void ctp_command_handler::go_to_next_command()
{
	// ʲô������Ҫ��
	//return;
	if (_currentCmd != nullptr)
	{
		boost::lock_guard<boost::mutex> guard(_buffer_mutex);

		if (_currentCmd->_type == CTP_COMMAND_TYPE::CTP_COMMAND_ENSURE_INS_DIR_NUMBERS)
		{
			_cmdBuffers.push_back(_cmdBuffers[0]);
		}
		_cmdBuffers.erase(_cmdBuffers.begin());
		_currentCmd = nullptr;
	}

	// TODO: ��ջ���
	//run();
}


void ctp_command_handler::execute_command_close(ctp_command &_cmd)
{
	LOG(DEBUG) << "Entering function " << __FUNCTION__;
	while (!_cmd._allArgus.empty())
	{
		ctp_command_argus &argu = _cmd._allArgus[0];
		
		
		if (argu._instrumentId.empty()
			|| argu._number < 1)
		{ 
			// ɾ����һ��
			_cmd._allArgus.erase(_cmd._allArgus.begin());
			continue;
		}
		// ����ط���Ҫ�Ķ���ƽ�ֵ�ʱ����Ҫ��ע�¾ɣ��п��ܳ��ֲ��������
		double price = argu._price;
		if (price == 0.0f) //ʹ�����ڵļ۸�
		{
			price = TradeContext::get_mutable_instance().m_allInstrumentRuntimes[argu._instrumentId].latestValue.close;
		}

		CloseInvestorPositions(_trade, argu._instrumentId, argu._direction, argu._number,price);

		// ɾ����һ��
		_cmd._allArgus.erase(_cmd._allArgus.begin());
		return;
	}

	// �ߵ��������û��Ҫִ�е��ˣ���Ϊ����Ĳ������첽������һ��ִ�в������ͷ���
	//go_to_next_command();
	LOG(DEBUG) << "Leaving function " << __FUNCTION__;
	return;
	//_trade.ReqOrderInsertExternal(_cmd)
}

void ctp_command_handler::CloseInvestorPositions(Trade &_trade, std::string _ins, CTP_COMMAND_ARGU_DIRECTION &_dir, int _number, float price)
{
	LOG(DEBUG) << "Entering function " << __FUNCTION__ <<"  "<< _trade.m_bRequestInProgress;
	//if (_trade.m_bRequestInProgress
	//	|| !_trade.m_bCompleteLoginProgress) return;
	//if(!_trade.CanTradeExternal(_ins.c_str())) return;

	if (TradeContext::get_mutable_instance().m_allInstrumentRuntimes.find(_ins)
		== TradeContext::get_mutable_instance().m_allInstrumentRuntimes.end()) return;

	InstrumentRuntime &insRuntime = TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_ins];
	//if (insRuntime.latestValue.close <= 1.0f) 
	//{
	//	LOG(INFO) << "No real time quote value for "<< insRuntime.instrumentID;
	//	return;//û��ֵ
	//}
	TwoDirectionsCThostFtdcInvestorPositionFieldMap::iterator itinvestor;
	for (itinvestor = insRuntime.investorPositionAllDirections.begin();
		itinvestor != insRuntime.investorPositionAllDirections.end();
		++itinvestor)
	{

		if (itinvestor->first != ConvertCmdArguDirectionToPosiDirection(_dir)) continue;

		int t_numbers = insRuntime.GetTodayNumberOfInvestorPositions(itinvestor->first);
		int numbers = insRuntime.GetTotalNumberOfInvestorPositions(itinvestor->first);

		if (numbers == 0) return;

		float closeprice = price;
		if(price ==0.0f)
		{
			closeprice = insRuntime.latestValue.close;
		}

		if (CancelFtdcOrder(_trade, _ins.c_str(), _dir))
		{
			LOG(INFO) << _ins << ":" << "ƽ,ȡ���ҵ���";
			return;
		}
		
		if (numbers - t_numbers >= _number)//��ƽ�ɲ�
		{
			LOG(INFO) << _ins << ":" << PosiDirectionDescription(itinvestor->first) << "ƽ�� " << _number;
			_trade.ReqOrderInsertExternal(_ins.c_str(), closeprice, ConvertPosiDirectionToCloseDirector(itinvestor->first), 1, _number);
			return;
		}
		else if(numbers > t_numbers)//�ɲ�ƽһ����,���ʱ��Ӧ����һ��ƽ�񣬵���û�취ͬ��ִ��
		{
			LOG(INFO) << _ins << ":" << PosiDirectionDescription(itinvestor->first) << "ƽ�� " << numbers - t_numbers;
			_trade.ReqOrderInsertExternal(_ins.c_str(), closeprice, ConvertPosiDirectionToCloseDirector(itinvestor->first), 1, numbers - t_numbers);
			return;
		}
		else if (t_numbers > _number)
		{
			LOG(INFO) << _ins << L":" << PosiDirectionDescription(itinvestor->first) << L"ƽ�� " << _number;
			_trade.ReqOrderInsertExternal(_ins.c_str(), closeprice, ConvertPosiDirectionToCloseDirector(itinvestor->first), 2, _number);
			return;
		}
		else
		{
			LOG(INFO) << _ins << ":" << PosiDirectionDescription(itinvestor->first) << "ƽ�� " << t_numbers;
			_trade.ReqOrderInsertExternal(_ins.c_str(), closeprice, ConvertPosiDirectionToCloseDirector(itinvestor->first), 2, t_numbers);
			return;
		}
	}

	LOG(DEBUG) << "Leaving function " << __FUNCTION__ << "  " << _trade.m_bRequestInProgress;
}

void ctp_command_handler::execute_command_open(ctp_command &_cmd)
{
	
	while (!_cmd._allArgus.empty())
	{
		ctp_command_argus &argu = _cmd._allArgus[0];

		//if (!_trade.CanTradeExternal(argu._instrumentId.c_str())) continue;

		if (argu._instrumentId.empty()
			|| argu._number < 1)
		{
			// ɾ����һ��
			_cmd._allArgus.erase(_cmd._allArgus.begin());
			continue;
		}
		// ����ط���Ҫ�Ķ���ƽ�ֵ�ʱ����Ҫ��ע�¾ɣ��п��ܳ��ֲ��������
		double price = argu._price;
		if (price == 0.0f) //ʹ�����ڵļ۸�
		{
			price = TradeContext::get_mutable_instance().m_allInstrumentRuntimes[argu._instrumentId].latestValue.close;
		}
		if (CancelFtdcOrder(_trade, argu._instrumentId.c_str(), argu._direction, true))
		{
			LOG(INFO) << argu._instrumentId << ":" << "��,ȡ���ҵ���";
			return;
		}
		LOG(INFO) << argu._instrumentId << ":"  << "�� " << OpenDirectionDescription(argu._direction) <<argu._number<<":"<<argu._price;
		

		_trade.ReqOrderInsertExternal(argu._instrumentId.c_str(), price, ConvertCmdArguDirectionToBuyDirector(argu._direction), 0, argu._number);

		// ɾ����һ��
		_cmd._allArgus.erase(_cmd._allArgus.begin());
		return;
	}

	// �ߵ��������û��Ҫִ�е��ˣ���Ϊ����Ĳ������첽������һ��ִ�в������ͷ���
	//go_to_next_command();
	return;
}
void ctp_command_handler::execute_command_close_all()
{
	

	std::map<std::string, InstrumentRuntime>::iterator it;
	for (it = TradeContext::get_mutable_instance().m_allInstrumentRuntimes.begin();
		it != TradeContext::get_mutable_instance().m_allInstrumentRuntimes.end();
		++it)
	{
		//if (!_trade.CanTradeExternal(it->first.c_str())) return;
		if (it->second.latestValue.close <= 1.0f)
		{
			LOG(INFO) << "No real time quote value for " << it->second.instrumentID;
			continue;//û��ֵ
		}
		TwoDirectionsCThostFtdcInvestorPositionFieldMap::iterator itinvestor;
		for (itinvestor = it->second.investorPositionAllDirections.begin();
			itinvestor != it->second.investorPositionAllDirections.end();
			++itinvestor)
		{
			int numbers = it->second.GetTodayNumberOfInvestorPositions(itinvestor->first);
			if (numbers > 0)
			{
				LOG(INFO) << it->first<<":"<< PosiDirectionDescription(itinvestor->first)<< "ƽ�� " << numbers;
				_trade.ReqOrderInsertExternal(it->first.c_str(),it->second.latestValue.close, ConvertPosiDirectionToCloseDirector(itinvestor->first), 2, numbers);
				return;
			}
			else
			{
				numbers = it->second.GetTotalNumberOfInvestorPositions(itinvestor->first);
				if (numbers > 0)
				{
					LOG(INFO) << it->first << ":" << PosiDirectionDescription(itinvestor->first) << "ƽ�� " << numbers;
					
					_trade.ReqOrderInsertExternal(it->first.c_str(),it->second.latestValue.close, ConvertPosiDirectionToCloseDirector(itinvestor->first), 1, numbers);
					return;
				}
			}
		}
	}

	//go_to_next_command();
}

void dump_context(Trade &_trade, const char * instrumentid,TThostFtdcPosiDirectionType _posidirection,int _strategi_number) {
	LOG(INFO) << "Trade can run external:" << _trade.CanTradeExternal();

	int numbers=TradeContext::get_mutable_instance().m_allInstrumentRuntimes[instrumentid].GetTotalNumberOfInvestorPositions(THOST_FTDC_PD_Long);
	LOG(INFO) << "�ѿ��൥��:" << numbers;
	numbers = TradeContext::get_mutable_instance().m_allInstrumentRuntimes[instrumentid].GetTotalNumberOfInvestorPositions(THOST_FTDC_PD_Short);
	LOG(INFO) << "�ѿ��յ���:" << numbers;

	LOG(INFO) << "����ֵ:" << TradeContext::get_mutable_instance().m_allInstrumentRuntimes[instrumentid].latestValue.close;

	if (_posidirection == THOST_FTDC_PD_Long) {
		LOG(INFO) << "���Զ൥��:" << _strategi_number;
	}else if (_posidirection == THOST_FTDC_PD_Short) {
		LOG(INFO) << "���Կյ���:" << _strategi_number;
	}
//#define THOST_FTDC_PD_Long '2'
//	///��ͷ
//#define THOST_FTDC_PD_Short '3'

}
void ctp_command_handler::execute_command_ensure(ctp_command &_cmd)
{
	if (_cmd._type != CTP_COMMAND_TYPE::CTP_COMMAND_ENSURE_INS_DIR_NUMBERS)
		return;
	
	
	if (_cmd._allArgus.empty()) 
		return;

	for (size_t _idx = 0; _idx < _cmd._allArgus.size(); ++_idx)
	{
		// disable״̬��ִ��
		if (!_cmd._allArgus[_idx]._enabled) continue;

		if (TradeContext::get_mutable_instance().m_allInstrumentRuntimes.find(_cmd._allArgus[_idx]._instrumentId)
			== TradeContext::get_mutable_instance().m_allInstrumentRuntimes.end())continue;//�Ҳ�����Լ�������ܷ���
		

		//�����ǰ��Լ��pengding�ģ���ȡ����
		
		// ת���ɶ������
		TThostFtdcPosiDirectionType type = ConvertCmdArguDirectionToPosiDirection(_cmd._allArgus[_idx]._direction);
		// �õ����е�����
		int _numbers = TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].GetTotalNumberOfInvestorPositions(type);

		if (_numbers < _cmd._allArgus[_idx]._number)//���ʱ��Ӧ�ÿ���
		{
			dump_context(_trade, _cmd._allArgus[_idx]._instrumentId.c_str(), type, _cmd._allArgus[_idx]._number);
			for (size_t i = 0; i < TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields.size(); ++i)
			{
				// ������ʱ���򵥾��Ƕ൥��ÿ��ֻ��ִ��һ������
				if (((_cmd._allArgus[_idx]._direction == CTP_COMMAND_ARGU_DIRECTION::BUY_DIRECTION
					&& TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.Direction == THOST_FTDC_D_Buy)
					|| (_cmd._allArgus[_idx]._direction == CTP_COMMAND_ARGU_DIRECTION::SELL_DIRECTION
						&& TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.Direction == THOST_FTDC_D_Sell))
					&& (TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.CombOffsetFlag[0] == THOST_FTDC_OF_Open))
				{
					LOG(INFO) << "ȡ�����ҵ� 1";
					_trade.ReqOrderActionExternal(_cmd._allArgus[_idx]._instrumentId.c_str(),
						TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.SessionID,
						TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.FrontID,
						TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.OrderRef);
					return;
				}
			}

			
			//if (!_trade.CanTradeExternal(_cmd._allArgus[_idx]._instrumentId.c_str())) continue;
			// TODO: close investor positions
			LOG(INFO) << _cmd._allArgus[_idx]._instrumentId << ":" << PosiDirectionDescription(ConvertCmdArguDirectionToPosiDirection(_cmd._allArgus[_idx]._direction)) << "����" << _cmd._allArgus[_idx]._number - _numbers << "��";
			_trade.ReqOrderInsertExternal(_cmd._allArgus[_idx]._instrumentId.c_str()
				, TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].latestValue.close
				, ConvertCmdArguDirectionToBuyDirector(_cmd._allArgus[_idx]._direction)
				, 0
				, _cmd._allArgus[_idx]._number - _numbers);
			return;
		}
		else if (_numbers > _cmd._allArgus[_idx]._number)
		{
			dump_context(_trade, _cmd._allArgus[_idx]._instrumentId.c_str(), type, _cmd._allArgus[_idx]._number);
			for (size_t i = 0; i < TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields.size(); ++i)
			{
				// ƽ�ֵ�ʱ�򣬶൥Ӧ�����������������෴����Ҫ���ԵĶ����ܶ࣬ÿ��ֻ��ִ��һ������
				if (((_cmd._allArgus[_idx]._direction == CTP_COMMAND_ARGU_DIRECTION::BUY_DIRECTION
					&& TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.Direction == THOST_FTDC_D_Sell)
					|| (_cmd._allArgus[_idx]._direction == CTP_COMMAND_ARGU_DIRECTION::SELL_DIRECTION
						&& TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.Direction == THOST_FTDC_D_Buy))
					&& (TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.CombOffsetFlag[0] == THOST_FTDC_OF_Close))
				{
					LOG(INFO) << "ȡ�����ҵ� 2";
					_trade.ReqOrderActionExternal(_cmd._allArgus[_idx]._instrumentId.c_str(),
						TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.SessionID,
						TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.FrontID,
						TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.OrderRef);
					return;
				}
			}

			//if (!_trade.CanTradeExternal(_cmd._allArgus[_idx]._instrumentId.c_str())) continue;
			LOG(INFO) << _cmd._allArgus[_idx]._instrumentId << ":"<< PosiDirectionDescription(ConvertCmdArguDirectionToPosiDirection(_cmd._allArgus[_idx]._direction))<<"ƽ��"<< _numbers - _cmd._allArgus[_idx]._number<<"��";
			CloseInvestorPositions(_trade,
				_cmd._allArgus[_idx]._instrumentId.c_str(),
				_cmd._allArgus[_idx]._direction,
				_numbers - _cmd._allArgus[_idx]._number,
				TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].latestValue.close);
			return;
		}
	}

	//go_to_next_command();
	
}
void ctp_command_handler::execute_list_all_cmd()
{
	if (!_trade.m_bCompleteLoginProgress) return;

	std::map<std::string, InstrumentRuntime>::iterator it;
	for (it = TradeContext::get_mutable_instance().m_allInstrumentRuntimes.begin();
		it != TradeContext::get_mutable_instance().m_allInstrumentRuntimes.end();
		++it)
	{
		TwoDirectionsCThostFtdcInvestorPositionFieldMap::iterator itinvestor;
		for (itinvestor = it->second.investorPositionAllDirections.begin();
			itinvestor != it->second.investorPositionAllDirections.end();
			++itinvestor)
		{
			int numbers = it->second.GetTotalNumberOfInvestorPositions(itinvestor->first);
			if (numbers > 0)
			{
				std::cout << it->first<<":"<< PosiDirectionDescription(itinvestor->first) << ":" << numbers << "��" << std::endl;
			}
		}
	}

	//go_to_next_command();
}

void ctp_command_handler::execute_command_lose_limit(ctp_command &_cmd)
{
	while (!_cmd._allArgus.empty())
	{
		ctp_command_argus &argu = _cmd._allArgus[0];


		if (argu._instrumentId.empty()
			|| argu._number < 1)
		{
			// ɾ����һ��
			_cmd._allArgus.erase(_cmd._allArgus.begin());
			continue;
		}
		// ����ط���Ҫ�Ķ���ƽ�ֵ�ʱ����Ҫ��ע�¾ɣ��п��ܳ��ֲ��������
		double price = argu._price;
		if (price == 0.0f) //ʹ�����ڵļ۸�
		{
			price = TradeContext::get_mutable_instance().m_allInstrumentRuntimes[argu._instrumentId].latestValue.close;
		}

		

		// ɾ����һ��
		_cmd._allArgus.erase(_cmd._allArgus.begin());
		return;
	}

	// �ߵ��������û��Ҫִ�е��ˣ���Ϊ����Ĳ������첽������һ��ִ�в������ͷ���
	//go_to_next_command();
	return;
	//_trade.ReqOrderInsertExternal(_cmd)
}

void ctp_command_handler::execute_command_win_limit(ctp_command &_cmd)
{
	while (!_cmd._allArgus.empty())
	{
		ctp_command_argus &argu = _cmd._allArgus[0];


		if (argu._instrumentId.empty()
			|| argu._number < 1)
		{
			// ɾ����һ��
			_cmd._allArgus.erase(_cmd._allArgus.begin());
			continue;
		}
		// ����ط���Ҫ�Ķ���ƽ�ֵ�ʱ����Ҫ��ע�¾ɣ��п��ܳ��ֲ��������
		double price = argu._price;
		if (price == 0.0f) //ʹ�����ڵļ۸�
		{
			price = TradeContext::get_mutable_instance().m_allInstrumentRuntimes[argu._instrumentId].latestValue.close;
		}

		

		// ɾ����һ��
		_cmd._allArgus.erase(_cmd._allArgus.begin());
		return;
	}

	// �ߵ��������û��Ҫִ�е��ˣ���Ϊ����Ĳ������첽������һ��ִ�в������ͷ���
	//go_to_next_command();
	return;
	//_trade.ReqOrderInsertExternal(_cmd)
}

// ˢ�³ֲ֣�����ʹ��
void ctp_command_handler::execute_refresh_ins_cmd(ctp_command &_cmd)
{
	for (size_t i = 0; i < _cmd._allArgus.size();++i)
	{
		if (!_cmd._allArgus[i]._instrumentId.empty())
		{
			_trade.reqQryInvestorPositionInQueue(_cmd._allArgus[i]._instrumentId.c_str());
		}
	}
	

	// �ߵ��������û��Ҫִ�е��ˣ���Ϊ����Ĳ������첽������һ��ִ�в������ͷ���
	//go_to_next_command();
	return;
}

bool ctp_command_handler::CancelFtdcOrder(Trade &_trade, const char* ins, CTP_COMMAND_ARGU_DIRECTION &_dir,bool _open )
{
	TThostFtdcDirectionType _type = THOST_FTDC_D_Sell;
	if (_dir == BUY_DIRECTION && _open)
		_type = THOST_FTDC_D_Buy;

	
	InstrumentRuntime &insRuntime = TradeContext::get_mutable_instance().m_allInstrumentRuntimes[ins];
	// ȡ���ҵ�
	for (size_t i = 0; i < insRuntime.pengdingFtdcOrderFields.size(); ++i)
	{
		
		if (insRuntime.pengdingFtdcOrderFields[i]._field.Direction == _type
			&& insRuntime.pengdingFtdcOrderFields[i]._field.VolumeTotal >0)//����δ�ɽ���
		{
			if ((!_open
				&& (insRuntime.pengdingFtdcOrderFields[i]._field.CombOffsetFlag[0] == THOST_FTDC_OF_Close
					|| insRuntime.pengdingFtdcOrderFields[i]._field.CombOffsetFlag[0] == THOST_FTDC_OF_CloseToday))
				|| (_open&& insRuntime.pengdingFtdcOrderFields[i]._field.CombOffsetFlag[0] == THOST_FTDC_OF_Open))
			{
				_trade.ReqOrderActionExternal(ins,
					insRuntime.pengdingFtdcOrderFields[i]._field.SessionID,
					insRuntime.pengdingFtdcOrderFields[i]._field.FrontID,
					insRuntime.pengdingFtdcOrderFields[i]._field.OrderRef);
				return true;
			}
		}
	}

	return false;
}

void ctp_command_handler::execute_command_cancelorder(ctp_command &_cmd)
{
	if (_cmd._type != CTP_COMMAND_CANCEL_ORDER)
	{
		// ɾ�����в���������ִ�����
		_cmd._allArgus.clear();
		return;
	}

	//if (_trade.m_bRequestInProgress) return;

	
	while (!_cmd._allArgus.empty())
	{
		ctp_command_argus & argu=_cmd._allArgus.front();

		std::map<std::string, InstrumentRuntime>::iterator it;
		for (it = TradeContext::get_mutable_instance().m_allInstrumentRuntimes.begin(); it != TradeContext::get_mutable_instance().m_allInstrumentRuntimes.end(); ++it)
		{
			for (size_t j = 0; j < it->second.pengdingFtdcOrderFields.size(); ++j)
			{
				for (std::vector<unsigned int>::iterator it1 = argu._order_hash.begin(); it1 != argu._order_hash.end(); ++it1)
					if (it->second.pengdingFtdcOrderFields[j]._hashCode == *it1)
					{
						_trade.ReqOrderActionExternal(
							it->second.pengdingFtdcOrderFields[j]._field.InstrumentID,
							it->second.pengdingFtdcOrderFields[j]._field.SessionID,
							it->second.pengdingFtdcOrderFields[j]._field.FrontID,
							it->second.pengdingFtdcOrderFields[j]._field.OrderRef);

						argu._order_hash.erase(it1);
						if (argu._order_hash.empty())
						{
							_cmd._allArgus.erase(_cmd._allArgus.begin());
						}
						return;
					}
			}
		}

		// �ߵ�����˵��û�ҵ���ɾ����������һ��
		_cmd._allArgus.erase(_cmd._allArgus.begin());
	}
	return;
	
}