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
	/*ֻ�����ĸ�ֵ*/
	void update_ctp_instrument_value_simple(CThostFtdcDepthMarketDataField *pField, const boost::function<void(const RedisValue &)> &handler);

	/*�õ�*/
	void get_ctp_instrument_value_simple(const char* instrumentid, const boost::function<void(const RedisValue &)> &handler);

	/*����������Ϣ*/
	void update_ctp_instrument_value_full(CThostFtdcDepthMarketDataField *pField);

	/*����redis�����е�instrument*/
	void update_ctp_instrument_ids();
	
	// ���е�updateÿ�ζ��滻
	// update login info
	void update_ctp_account_login_info(const char * investor, const char*str);

	// after login, update account information
	void update_ctp_account_info(const CThostFtdcTradingAccountField &pField);

	// �ֲ�
	void update_ctp_CThostFtdcInvestorPositionField(const char * investor, const char* instrumentid, const TwoDirectionsCThostFtdcInvestorPositionFieldMap &fields);

	// �����������������б�����ʷ��pending��
	void update_ctp_CThostFtdcOrderField(const char* suffix,const char* investor, const char* instrumentid, const std::vector<MyCThostFtdcOrderField> &fields);

	// �ɽ���¼
	void update_ctp_CThostFtdcTradeField(const char* investor, const char* instrumentid, const std::vector<CThostFtdcTradeField> &fields);


	//�����⼸����Ӧ���ڵ�ǰ�˻��ģ���������ķ���
	// ��������
	void update_ctp_CThostFtdcInputOrderField(const char* investor, const char* instrumentid, const std::vector<CThostFtdcInputOrderField> &fields);

	// �ֲ�
	void update_ctp_CThostFtdcInvestorPositionField(const char * investor);

	// ����
	void update_ctp_CThostFtdcOrderField(const char* investor);

	// �ɽ���¼
	void update_ctp_CThostFtdcTradeField(const char* investor);

	// ��������
	void update_ctp_CThostFtdcInputOrderField(const char* investor);


	/*�õ�redis�����е�instrument*/
	void get_ctp_instrument_ids(const boost::function<void(const RedisValue &)> &handler);
};

#endif // !CTP_REDIS_WORKER_H__