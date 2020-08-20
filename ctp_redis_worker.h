#ifndef CTP_REDIS_WORKER_H__
#define CTP_REDIS_WORKER_H__
#include "redisclient.h"
#include <map>
#include <vector>
//#include "ThostFtdcTraderApi.h"
#include "InstrumentRuntime.h"
class ctp_redis_worker
{
public:
	ctp_redis_worker(RedisClient &_redis);
	~ctp_redis_worker();

private:
	RedisClient &redis;
public:
	/*只包含四个值*/
	void update_ctp_instrument_value_simple(CThostFtdcDepthMarketDataField *pField, const boost::function<void(const RedisValue &)> &handler);

	/*得到*/
	void get_ctp_instrument_value_simple(const char* instrumentid, const boost::function<void(const RedisValue &)> &handler);

	/*包含所有信息*/
	void update_ctp_instrument_value_full(CThostFtdcDepthMarketDataField *pField);

	/*更新redis中所有的instrument*/
	void update_ctp_instrument_ids();
	
	// 所有的update每次都替换
	// update login info
	void update_ctp_account_login_info(const char * investor, const char*str);

	// after login, update account information
	void update_ctp_account_info(const CThostFtdcTradingAccountField &pField);

	// 持仓
	void update_ctp_CThostFtdcInvestorPositionField(const char * investor, const char* instrumentid, const TwoDirectionsCThostFtdcInvestorPositionFieldMap &fields);

	// 报单报单，包括所有报单历史和pending的
	void update_ctp_CThostFtdcOrderField(const char* suffix,const char* investor, const char* instrumentid, const std::vector<MyCThostFtdcOrderField> &fields);

	// 成交记录
	void update_ctp_CThostFtdcTradeField(const char* investor, const char* instrumentid, const std::vector<CThostFtdcTradeField> &fields);


	//下面这几个是应用于当前账户的，调用上面的方法
	// 报单错误
	void update_ctp_CThostFtdcInputOrderField(const char* investor, const char* instrumentid, const std::vector<CThostFtdcInputOrderField> &fields);

	// 持仓
	void update_ctp_CThostFtdcInvestorPositionField(const char * investor);

	// 报单
	void update_ctp_CThostFtdcOrderField(const char* investor);

	// 成交记录
	void update_ctp_CThostFtdcTradeField(const char* investor);

	// 报单错误
	void update_ctp_CThostFtdcInputOrderField(const char* investor);


	/*得到redis中所有的instrument*/
	void get_ctp_instrument_ids(const boost::function<void(const RedisValue &)> &handler);
};

#endif // !CTP_REDIS_WORKER_H__