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

	//满足下面三个条件之一，等待200ms
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
		case CTP_COMMAND_TYPE::CTP_COMMAND_ENSURE_INS_DIR_NUMBERS://如果没有的，不去检查。而不是所有的都平仓
			buffer_ensure_ins_cmd(_command);
			break;
		default:
			{
				boost::lock_guard<boost::mutex> guard(_buffer_mutex);
				_cmdBuffers.push_back(_command);
			}
			break;
		}

		// 立刻出发
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

	// 循环遍历所有的参数，可能包含多个合约
	for (size_t _c_argu_idx = 0; _c_argu_idx < _command._allArgus.size(); ++_c_argu_idx)
	{
		size_t buf_idx;
		{
			boost::lock_guard<boost::mutex> guard(_buffer_mutex);
			for (buf_idx = 0; buf_idx < _cmdBuffers.size(); ++buf_idx)
			{
				if (_cmdBuffers[buf_idx]._allArgus.size() == 0) continue;
				// 当前合约的该命令
				if (_cmdBuffers[buf_idx]._type == _command._type
					&& _cmdBuffers[buf_idx]._allArgus[0]._instrumentId.compare(_command._allArgus[_c_argu_idx]._instrumentId) == 0)//没有的应该不会加进来
				{
					size_t argu_idx;
					for (argu_idx = 0; argu_idx < _cmdBuffers[buf_idx]._allArgus.size(); ++argu_idx)
					{
						if (_cmdBuffers[buf_idx]._allArgus[argu_idx]._direction == _command._allArgus[_c_argu_idx]._direction)//找到某个方向的
						{
							_cmdBuffers[buf_idx]._allArgus[argu_idx] = _command._allArgus[_c_argu_idx];
							break;
						}
					}

					if (argu_idx == _cmdBuffers[buf_idx]._allArgus.size()
						&& _command._allArgus[_c_argu_idx]._enabled)//没有找到,而且是enabled
					{
						_cmdBuffers[buf_idx]._allArgus.push_back(_command._allArgus[_c_argu_idx]);
					}

					// 找到了
					break;
				}
			}
			if (buf_idx == _cmdBuffers.size()
				&& _command._allArgus[_c_argu_idx]._enabled)//没找到
			{
				ctp_command _cmd;
				_cmd._type = _command._type;
				_cmd._allArgus.push_back(_command._allArgus[_c_argu_idx]);
				_cmdBuffers.push_back(_cmd);
			}
		}

		
	}
}

// 这个方法要不要改成循环执行？
void ctp_command_handler::run()
{
	//LOG(DEBUG) <<"Entering function "<< __FUNCTION__;
	{
		boost::lock_guard<boost::mutex> guard(_buffer_mutex);
		// 缓存中有指令，而且指令中有操作的话，立刻执行
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
	//先实现这一个
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
		//平仓
		execute_command_close(*_currentCmd);
	}
	else if (_currentCmd->_type == CTP_COMMAND_TYPE::CTP_COMMAND_OPEN)
	{
		//开仓
		execute_command_open(*_currentCmd);
	}
	else if (_currentCmd->_type == CTP_COMMAND_TYPE::CTP_COMMAND_ENSURE_INS_DIR_NUMBERS)//
	{
		// 保障单，只此一家别无分号，适合追单
		execute_command_ensure(*_currentCmd);
	}
	else if (_currentCmd->_type == CTP_COMMAND_TYPE::CTP_COMMAND_LOSS_LIMIT)
	{
		// 止损,不实现,属于业务层面
		//execute_command_lose_limit(*_currentCmd);
	}
	else if (_currentCmd->_type == CTP_COMMAND_TYPE::CTP_COMMAND_WIN_LIMIT)
	{
		// 止盈,不实现,属于业务层面
		//execute_command_win_limit(*_currentCmd);
	}
	else if (_currentCmd->_type == CTP_COMMAND_TYPE::CTP_COMMAND_RERESH_INS)
	{
		execute_refresh_ins_cmd(*_currentCmd);
	}
	else if (_currentCmd->_type == CTP_COMMAND_TYPE::CTP_COMMAND_CANCEL_ORDER)
	{
		//平仓
		execute_command_cancelorder(*_currentCmd);
	}

	//所有的走完都这样
	_currentCmd->_allArgus.clear();
	go_to_next_command();
	//LOG(DEBUG) << "Leave function " << __FUNCTION__;
}
void ctp_command_handler::go_to_next_command()
{
	// 什么都不需要做
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

	// TODO: 堆栈溢出
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
			// 删掉第一个
			_cmd._allArgus.erase(_cmd._allArgus.begin());
			continue;
		}
		// 这个地方需要改动，平仓的时候不需要关注新旧，有可能出现不够的情况
		double price = argu._price;
		if (price == 0.0f) //使用现在的价格
		{
			price = TradeContext::get_mutable_instance().m_allInstrumentRuntimes[argu._instrumentId].latestValue.close;
		}

		CloseInvestorPositions(_trade, argu._instrumentId, argu._direction, argu._number,price);

		// 删掉第一个
		_cmd._allArgus.erase(_cmd._allArgus.begin());
		return;
	}

	// 走到这里表明没有要执行的了，因为里面的操作上异步操作，一旦执行操作，就返回
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
	//	return;//没有值
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
			LOG(INFO) << _ins << ":" << "平,取消挂单先";
			return;
		}
		
		if (numbers - t_numbers >= _number)//先平旧仓
		{
			LOG(INFO) << _ins << ":" << PosiDirectionDescription(itinvestor->first) << "平旧 " << _number;
			_trade.ReqOrderInsertExternal(_ins.c_str(), closeprice, ConvertPosiDirectionToCloseDirector(itinvestor->first), 1, _number);
			return;
		}
		else if(numbers > t_numbers)//旧仓平一部分,这个时候应该下一步平今，但是没办法同步执行
		{
			LOG(INFO) << _ins << ":" << PosiDirectionDescription(itinvestor->first) << "平旧 " << numbers - t_numbers;
			_trade.ReqOrderInsertExternal(_ins.c_str(), closeprice, ConvertPosiDirectionToCloseDirector(itinvestor->first), 1, numbers - t_numbers);
			return;
		}
		else if (t_numbers > _number)
		{
			LOG(INFO) << _ins << L":" << PosiDirectionDescription(itinvestor->first) << L"平今 " << _number;
			_trade.ReqOrderInsertExternal(_ins.c_str(), closeprice, ConvertPosiDirectionToCloseDirector(itinvestor->first), 2, _number);
			return;
		}
		else
		{
			LOG(INFO) << _ins << ":" << PosiDirectionDescription(itinvestor->first) << "平今 " << t_numbers;
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
			// 删掉第一个
			_cmd._allArgus.erase(_cmd._allArgus.begin());
			continue;
		}
		// 这个地方需要改动，平仓的时候不需要关注新旧，有可能出现不够的情况
		double price = argu._price;
		if (price == 0.0f) //使用现在的价格
		{
			price = TradeContext::get_mutable_instance().m_allInstrumentRuntimes[argu._instrumentId].latestValue.close;
		}
		if (CancelFtdcOrder(_trade, argu._instrumentId.c_str(), argu._direction, true))
		{
			LOG(INFO) << argu._instrumentId << ":" << "开,取消挂单先";
			return;
		}
		LOG(INFO) << argu._instrumentId << ":"  << "开 " << OpenDirectionDescription(argu._direction) <<argu._number<<":"<<argu._price;
		

		_trade.ReqOrderInsertExternal(argu._instrumentId.c_str(), price, ConvertCmdArguDirectionToBuyDirector(argu._direction), 0, argu._number);

		// 删掉第一个
		_cmd._allArgus.erase(_cmd._allArgus.begin());
		return;
	}

	// 走到这里表明没有要执行的了，因为里面的操作上异步操作，一旦执行操作，就返回
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
			continue;//没有值
		}
		TwoDirectionsCThostFtdcInvestorPositionFieldMap::iterator itinvestor;
		for (itinvestor = it->second.investorPositionAllDirections.begin();
			itinvestor != it->second.investorPositionAllDirections.end();
			++itinvestor)
		{
			int numbers = it->second.GetTodayNumberOfInvestorPositions(itinvestor->first);
			if (numbers > 0)
			{
				LOG(INFO) << it->first<<":"<< PosiDirectionDescription(itinvestor->first)<< "平今 " << numbers;
				_trade.ReqOrderInsertExternal(it->first.c_str(),it->second.latestValue.close, ConvertPosiDirectionToCloseDirector(itinvestor->first), 2, numbers);
				return;
			}
			else
			{
				numbers = it->second.GetTotalNumberOfInvestorPositions(itinvestor->first);
				if (numbers > 0)
				{
					LOG(INFO) << it->first << ":" << PosiDirectionDescription(itinvestor->first) << "平旧 " << numbers;
					
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
	LOG(INFO) << "已开多单数:" << numbers;
	numbers = TradeContext::get_mutable_instance().m_allInstrumentRuntimes[instrumentid].GetTotalNumberOfInvestorPositions(THOST_FTDC_PD_Short);
	LOG(INFO) << "已开空单数:" << numbers;

	LOG(INFO) << "最新值:" << TradeContext::get_mutable_instance().m_allInstrumentRuntimes[instrumentid].latestValue.close;

	if (_posidirection == THOST_FTDC_PD_Long) {
		LOG(INFO) << "策略多单数:" << _strategi_number;
	}else if (_posidirection == THOST_FTDC_PD_Short) {
		LOG(INFO) << "策略空单数:" << _strategi_number;
	}
//#define THOST_FTDC_PD_Long '2'
//	///空头
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
		// disable状态不执行
		if (!_cmd._allArgus[_idx]._enabled) continue;

		if (TradeContext::get_mutable_instance().m_allInstrumentRuntimes.find(_cmd._allArgus[_idx]._instrumentId)
			== TradeContext::get_mutable_instance().m_allInstrumentRuntimes.end())continue;//找不到合约，不可能发生
		

		//如果当前合约有pengding的，先取消掉
		
		// 转化成多空类型
		TThostFtdcPosiDirectionType type = ConvertCmdArguDirectionToPosiDirection(_cmd._allArgus[_idx]._direction);
		// 得到所有的数量
		int _numbers = TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].GetTotalNumberOfInvestorPositions(type);

		if (_numbers < _cmd._allArgus[_idx]._number)//这个时候应该开仓
		{
			dump_context(_trade, _cmd._allArgus[_idx]._instrumentId.c_str(), type, _cmd._allArgus[_idx]._number);
			for (size_t i = 0; i < TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields.size(); ++i)
			{
				// 开单的时候，买单就是多单，每次只能执行一个操作
				if (((_cmd._allArgus[_idx]._direction == CTP_COMMAND_ARGU_DIRECTION::BUY_DIRECTION
					&& TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.Direction == THOST_FTDC_D_Buy)
					|| (_cmd._allArgus[_idx]._direction == CTP_COMMAND_ARGU_DIRECTION::SELL_DIRECTION
						&& TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.Direction == THOST_FTDC_D_Sell))
					&& (TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.CombOffsetFlag[0] == THOST_FTDC_OF_Open))
				{
					LOG(INFO) << "取消个挂单 1";
					_trade.ReqOrderActionExternal(_cmd._allArgus[_idx]._instrumentId.c_str(),
						TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.SessionID,
						TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.FrontID,
						TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.OrderRef);
					return;
				}
			}

			
			//if (!_trade.CanTradeExternal(_cmd._allArgus[_idx]._instrumentId.c_str())) continue;
			// TODO: close investor positions
			LOG(INFO) << _cmd._allArgus[_idx]._instrumentId << ":" << PosiDirectionDescription(ConvertCmdArguDirectionToPosiDirection(_cmd._allArgus[_idx]._direction)) << "开仓" << _cmd._allArgus[_idx]._number - _numbers << "手";
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
				// 平仓的时候，多单应该是卖掉，跟上面相反，需要测试的东西很多，每次只能执行一个操作
				if (((_cmd._allArgus[_idx]._direction == CTP_COMMAND_ARGU_DIRECTION::BUY_DIRECTION
					&& TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.Direction == THOST_FTDC_D_Sell)
					|| (_cmd._allArgus[_idx]._direction == CTP_COMMAND_ARGU_DIRECTION::SELL_DIRECTION
						&& TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.Direction == THOST_FTDC_D_Buy))
					&& (TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.CombOffsetFlag[0] == THOST_FTDC_OF_Close))
				{
					LOG(INFO) << "取消个挂单 2";
					_trade.ReqOrderActionExternal(_cmd._allArgus[_idx]._instrumentId.c_str(),
						TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.SessionID,
						TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.FrontID,
						TradeContext::get_mutable_instance().m_allInstrumentRuntimes[_cmd._allArgus[_idx]._instrumentId].pengdingFtdcOrderFields[i]._field.OrderRef);
					return;
				}
			}

			//if (!_trade.CanTradeExternal(_cmd._allArgus[_idx]._instrumentId.c_str())) continue;
			LOG(INFO) << _cmd._allArgus[_idx]._instrumentId << ":"<< PosiDirectionDescription(ConvertCmdArguDirectionToPosiDirection(_cmd._allArgus[_idx]._direction))<<"平仓"<< _numbers - _cmd._allArgus[_idx]._number<<"手";
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
				std::cout << it->first<<":"<< PosiDirectionDescription(itinvestor->first) << ":" << numbers << "手" << std::endl;
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
			// 删掉第一个
			_cmd._allArgus.erase(_cmd._allArgus.begin());
			continue;
		}
		// 这个地方需要改动，平仓的时候不需要关注新旧，有可能出现不够的情况
		double price = argu._price;
		if (price == 0.0f) //使用现在的价格
		{
			price = TradeContext::get_mutable_instance().m_allInstrumentRuntimes[argu._instrumentId].latestValue.close;
		}

		

		// 删掉第一个
		_cmd._allArgus.erase(_cmd._allArgus.begin());
		return;
	}

	// 走到这里表明没有要执行的了，因为里面的操作上异步操作，一旦执行操作，就返回
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
			// 删掉第一个
			_cmd._allArgus.erase(_cmd._allArgus.begin());
			continue;
		}
		// 这个地方需要改动，平仓的时候不需要关注新旧，有可能出现不够的情况
		double price = argu._price;
		if (price == 0.0f) //使用现在的价格
		{
			price = TradeContext::get_mutable_instance().m_allInstrumentRuntimes[argu._instrumentId].latestValue.close;
		}

		

		// 删掉第一个
		_cmd._allArgus.erase(_cmd._allArgus.begin());
		return;
	}

	// 走到这里表明没有要执行的了，因为里面的操作上异步操作，一旦执行操作，就返回
	//go_to_next_command();
	return;
	//_trade.ReqOrderInsertExternal(_cmd)
}

// 刷新持仓，测试使用
void ctp_command_handler::execute_refresh_ins_cmd(ctp_command &_cmd)
{
	for (size_t i = 0; i < _cmd._allArgus.size();++i)
	{
		if (!_cmd._allArgus[i]._instrumentId.empty())
		{
			_trade.reqQryInvestorPositionInQueue(_cmd._allArgus[i]._instrumentId.c_str());
		}
	}
	

	// 走到这里表明没有要执行的了，因为里面的操作上异步操作，一旦执行操作，就返回
	//go_to_next_command();
	return;
}

bool ctp_command_handler::CancelFtdcOrder(Trade &_trade, const char* ins, CTP_COMMAND_ARGU_DIRECTION &_dir,bool _open )
{
	TThostFtdcDirectionType _type = THOST_FTDC_D_Sell;
	if (_dir == BUY_DIRECTION && _open)
		_type = THOST_FTDC_D_Buy;

	
	InstrumentRuntime &insRuntime = TradeContext::get_mutable_instance().m_allInstrumentRuntimes[ins];
	// 取消挂单
	for (size_t i = 0; i < insRuntime.pengdingFtdcOrderFields.size(); ++i)
	{
		
		if (insRuntime.pengdingFtdcOrderFields[i]._field.Direction == _type
			&& insRuntime.pengdingFtdcOrderFields[i]._field.VolumeTotal >0)//还有未成交的
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
		// 删掉所有参数，表明执行完成
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

		// 走到这里说明没找到，删掉，继续下一个
		_cmd._allArgus.erase(_cmd._allArgus.begin());
	}
	return;
	
}