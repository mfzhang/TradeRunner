#ifndef INSTRUMENTJSONHELPER_H__
#define INSTRUMENTJSONHELPER_H__
#include <string>
#include <map>
#include <vector>
#include "InstrumentRuntime.h"
#include "json\json.h"
class instrument_json_helper
{
private:
	instrument_json_helper();
	~instrument_json_helper();
public:
	/* 账户信息,实时更新账户信息，判断是否在线*/
	static void SerializeJsonCThostFtdcTradingAccountField(Json::Value &value, const CThostFtdcTradingAccountField &pField);
	/* serialize 持仓*/
	static void SerializeJsonCThostFtdcInvestorPositionField(Json::Value &value,const CThostFtdcInvestorPositionField &pField);
	static void SerializeJsonCThostFtdcInvestorPositionField(Json::Value &value, const std::vector<CThostFtdcInvestorPositionField > &fields);
	static void SerializeJsonCThostFtdcInvestorPositionField(Json::Value &value, const TwoDirectionsCThostFtdcInvestorPositionFieldMap &fields);
	/* deserialize 持仓*/
	static CThostFtdcInvestorPositionField DeserializeCThostFtdcInvestorPositionField(const std::string &field);
	static CThostFtdcInvestorPositionField DeserializeCThostFtdcInvestorPositionField(const Json::Value &root);
	static void DeserializeCThostFtdcInvestorPositionFields(TwoDirectionsCThostFtdcInvestorPositionFieldMap &fields, const std::string &content);


	/*报单*/
	static void SerializeJsonCThostFtdcOrderField(Json::Value &root, const MyCThostFtdcOrderField &pField);
	static void SerializeJsonCThostFtdcOrderField(Json::Value &value, const std::vector<MyCThostFtdcOrderField> &fields);
	/*deserialize报单*/
	static MyCThostFtdcOrderField DeserializeJsonCThostFtdcOrderField(const std::string &field);
	static MyCThostFtdcOrderField DeserializeJsonCThostFtdcOrderField(Json::Value &root);
	static void DeserializeJsonCThostFtdcOrderField(std::vector<MyCThostFtdcOrderField> &fields,Json::Value &value, const std::string &content);

	/*报单错误*/
	static void SerializeJsonCThostFtdcInputOrderField(Json::Value &root, const CThostFtdcInputOrderField &pField);
	static void SerializeJsonCThostFtdcInputOrderField(Json::Value &value, const std::vector<CThostFtdcInputOrderField> &fields);
	/*deserialize报单错误*/
	static CThostFtdcInputOrderField DeserializeJsonCThostFtdcInputOrderField(const std::string &field);
	static CThostFtdcInputOrderField DeserializeJsonCThostFtdcInputOrderField(Json::Value &root);
	static void DeserializeJsonCThostFtdcInputOrderField(std::vector<CThostFtdcInputOrderField> &fields, Json::Value &value, const std::string &content);


	/* 成交记录*/
	static void SerializeJsonCThostFtdcTradeField(Json::Value &root, const CThostFtdcTradeField &pField);
	static void SerializeJsonCThostFtdcTradeField(Json::Value &value, const std::vector<CThostFtdcTradeField> &fields);
	/*deserialize成交记录*/
	static CThostFtdcTradeField DeserializeJsonCThostFtdcTradeField(const std::string &field);
	static CThostFtdcTradeField DeserializeJsonCThostFtdcTradeField(Json::Value &root);
	static void DeserializeJsonCThostFtdcTradeField(std::vector<CThostFtdcTradeField > &fields, Json::Value &value, const std::string &content);

	// InstrumentRuntime
	static void SerializeJsonInstrumentRuntime(Json::Value &root, const InstrumentRuntime & instrumentruntime);

	//instrument basic info
	/*static void SerializeJsonCThostFtdcInstrumentField(Json::Value &root, const CThostFtdcInstrumentField *pField)*/;

};

#endif // !INSTRUMENTJSONHELPER_H__