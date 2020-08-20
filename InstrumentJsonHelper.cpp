#include "InstrumentJsonHelper.h"
instrument_json_helper::instrument_json_helper()
{
}


instrument_json_helper::~instrument_json_helper()
{
}

void instrument_json_helper::SerializeJsonCThostFtdcTradingAccountField(Json::Value &root, const CThostFtdcTradingAccountField &pField)
{
	root["BrokerID"] = pField.BrokerID;
	root["AccountID"] = pField.AccountID;
	root["PreMortgage"] = pField.PreMortgage;
	root["PreCredit"] = pField.PreCredit;
	root["PreDeposit"] = pField.PreDeposit;
	root["PreBalance"] = pField.PreBalance;
	root["PreMargin"] = pField.PreMargin;
	root["InterestBase"] = pField.InterestBase;
	root["Interest"] = pField.Interest;
	root["Deposit"] = pField.Deposit;
	root["Withdraw"] = pField.Withdraw;
	root["FrozenMargin"] = pField.FrozenMargin;
	root["FrozenCash"] = pField.FrozenCash;
	root["FrozenCommission"] = pField.FrozenCommission;
	root["CurrMargin"] = pField.CurrMargin;
	root["CashIn"] = pField.CashIn;
	root["Commission"] = pField.Commission;
	root["CloseProfit"] = pField.CloseProfit;
	root["FrozenCash"] = pField.FrozenCash;
	root["FrozenCommission"] = pField.FrozenCommission;
	root["CashIn"] = pField.CashIn;
	root["Commission"] = pField.Commission;
	root["CloseProfit"] = pField.CloseProfit;
	root["PositionProfit"] = pField.PositionProfit;
	root["Balance"] = pField.Balance;
	root["Available"] = pField.Available;
	root["WithdrawQuota"] = pField.WithdrawQuota;
	root["Reserve"] = pField.Reserve;
	root["TradingDay"] = pField.TradingDay;
	root["SettlementID"] = pField.SettlementID;
	root["Credit"] = pField.Credit;
	root["Mortgage"] = pField.Mortgage;
	root["ExchangeMargin"] = pField.ExchangeMargin;
	root["DeliveryMargin"] = pField.DeliveryMargin;
	root["ExchangeDeliveryMargin"] = pField.ExchangeDeliveryMargin;
}
/***持仓序列化***/
void instrument_json_helper::SerializeJsonCThostFtdcInvestorPositionField(Json::Value &root,const CThostFtdcInvestorPositionField &pField)
{
	
	root["InstrumentID"] = pField.InstrumentID;
	root["BrokerID"] = pField.BrokerID;
	root["InvestorID"] = pField.InvestorID;
	root["PosiDirection"] = pField.PosiDirection;
	root["HedgeFlag"] = pField.HedgeFlag;
	root["PositionDate"] = pField.PositionDate;
	root["YdPosition"] = pField.YdPosition;
	root["Position"] = pField.Position;
	root["LongFrozen"] = pField.LongFrozen;
	root["ShortFrozen"] = pField.ShortFrozen;
	root["LongFrozenAmount"] = pField.LongFrozenAmount;
	root["ShortFrozenAmount"] = pField.ShortFrozenAmount;
	root["OpenVolume"] = pField.OpenVolume;
	root["CloseVolume"] = pField.CloseVolume;
	root["OpenAmount"] = pField.OpenAmount;
	root["CloseAmount"] = pField.CloseAmount;
	root["PositionCost"] = pField.PositionCost;
	root["PreMargin"] = pField.PreMargin;
	root["UseMargin"] = pField.UseMargin;
	root["FrozenCash"] = pField.FrozenCash;
	root["FrozenCommission"] = pField.FrozenCommission;
	root["CashIn"] = pField.CashIn;
	root["Commission"] = pField.Commission;
	root["CloseProfit"] = pField.CloseProfit;
	root["PositionProfit"] = pField.PositionProfit;
	root["PreSettlementPrice"] = pField.PreSettlementPrice;
	root["SettlementPrice"] = pField.SettlementPrice;
	root["TradingDay"] = pField.TradingDay;
	root["OpenCost"] = pField.OpenCost;
	root["ExchangeMargin"] = pField.ExchangeMargin;
	root["CombPosition"] = pField.CombPosition;
	root["CombLongFrozen"] = pField.CombLongFrozen;
	root["CombShortFrozen"] = pField.CombShortFrozen;
	root["CloseProfitByDate"] = pField.CloseProfitByDate;
	root["CloseProfitByTrade"] = pField.CloseProfitByTrade;
	root["TodayPosition"] = pField.TodayPosition;
	root["MarginRateByMoney"] = pField.MarginRateByMoney;
	root["MarginRateByVolume"] = pField.MarginRateByVolume;
}


void instrument_json_helper::SerializeJsonCThostFtdcInvestorPositionField(Json::Value &value, const std::vector<CThostFtdcInvestorPositionField > &fields)
{
	for (size_t i = 0; i < fields.size(); ++i)
	{
		Json::Value val;
		SerializeJsonCThostFtdcInvestorPositionField(val, fields[i]);
		value.append(val);
	}
}

void instrument_json_helper::SerializeJsonCThostFtdcInvestorPositionField(Json::Value &value, const TwoDirectionsCThostFtdcInvestorPositionFieldMap &fields)
{
	TwoDirectionsCThostFtdcInvestorPositionFieldMap::const_iterator it;
	for (it = fields.begin(); it != fields.end(); ++it)
	{
		Json::Value val;
		val["direction"] = it->first;
		Json::Value vec(Json::ValueType::arrayValue);
		SerializeJsonCThostFtdcInvestorPositionField(vec,it->second);
		val["fields"] = vec;
		
		value.append(val);
	}
}


/***持仓序反列化***/
CThostFtdcInvestorPositionField instrument_json_helper::DeserializeCThostFtdcInvestorPositionField(const Json::Value &root)
{
	CThostFtdcInvestorPositionField pField = { 0 };
	if (root.size() <= 0) return pField;

	
	strncpy_s(pField.InstrumentID, root["InstrumentID"].asCString(), sizeof(TThostFtdcInstrumentIDType) - 1);
	strncpy_s(pField.BrokerID, root["BrokerID"].asCString(), sizeof(TThostFtdcBrokerIDType) - 1);
	strncpy_s(pField.InvestorID, root["InvestorID"].asCString(), sizeof(TThostFtdcInvestorIDType) - 1);
	pField.PosiDirection = *root["PosiDirection"].asCString();
	pField.HedgeFlag = *root["HedgeFlag"].asCString();
	pField.PositionDate = *root["PositionDate"].asCString();
	pField.YdPosition = root["YdPosition"].asInt();
	pField.Position = root["Position"].asInt();
	pField.LongFrozen = root["LongFrozen"].asInt();
	pField.ShortFrozen = root["ShortFrozen"].asInt();
	pField.LongFrozenAmount = root["LongFrozenAmount"].asDouble();
	pField.ShortFrozenAmount = root["ShortFrozenAmount"].asDouble();
	pField.OpenVolume = root["OpenVolume"].asInt();
	pField.OpenAmount = root["OpenAmount"].asDouble();
	pField.CloseAmount = root["CloseAmount"].asDouble();
	pField.PositionCost = root["PositionCost"].asDouble();
	pField.PreMargin = root["PreMargin"].asDouble();
	pField.UseMargin = root["UseMargin"].asDouble();
	pField.FrozenCash = root["FrozenCash"].asDouble();
	pField.FrozenCommission = root["FrozenCommission"].asDouble();
	pField.CashIn = root["CashIn"].asDouble();
	pField.Commission = root["Commission"].asDouble();
	pField.CloseProfit = root["CloseProfit"].asDouble();

	pField.PositionProfit = root["PositionProfit"].asDouble();
	pField.PreSettlementPrice = root["PreSettlementPrice"].asDouble();

	pField.SettlementPrice = root["SettlementPrice"].asDouble();
	strncpy_s(pField.TradingDay, root["TradingDay"].asCString(), sizeof(TThostFtdcDateType) - 1);
	pField.OpenCost = root["OpenCost"].asDouble();
	pField.ExchangeMargin = root["ExchangeMargin"].asDouble();
	pField.CombPosition = root["CombPosition"].asInt();
	pField.CombLongFrozen = root["CombLongFrozen"].asInt();
	pField.CombShortFrozen = root["CombShortFrozen"].asInt();
	pField.CloseProfitByDate = root["CloseProfitByDate"].asDouble();
	pField.CloseProfitByTrade = root["CloseProfitByTrade"].asDouble();
	pField.TodayPosition = root["TodayPosition"].asInt();
	pField.MarginRateByMoney = root["MarginRateByMoney"].asDouble();
	pField.MarginRateByVolume = root["MarginRateByVolume"].asDouble();

	return pField;
}
CThostFtdcInvestorPositionField instrument_json_helper::DeserializeCThostFtdcInvestorPositionField(const std::string &field)
{
	CThostFtdcInvestorPositionField pField = {0};
	Json::Reader parser;
	Json::Value root;
	if (parser.parse(field, root, false))
	{
		if (root.isArray()) return pField;
		return DeserializeCThostFtdcInvestorPositionField(root);
	}
	return pField;
}


void instrument_json_helper::DeserializeCThostFtdcInvestorPositionFields(TwoDirectionsCThostFtdcInvestorPositionFieldMap &fields, const std::string &content)
{
	if (content.empty()) return ;
	Json::Reader parser;
	Json::Value root;
	if (parser.parse(content, root, false))
	{
		if (root.isArray())
		{
			for (Json::ArrayIndex i = 0; i < root.size(); ++i)
			{
				TThostFtdcPosiDirectionType direction = *(root[i]["direction"].asCString());
				if (root[i]["array"].isArray())
				{
					for (Json::ArrayIndex  j = 0; j < root[i]["array"].size(); ++j)
					{
						CThostFtdcInvestorPositionField pField = DeserializeCThostFtdcInvestorPositionField(root[i]["array"][j]);
						if (strlen(pField.InstrumentID) != 0)
						{
							fields[direction].push_back(pField);
						}
					}
				}
			}
		}
	}
}

/***报单序列化***/
void instrument_json_helper::SerializeJsonCThostFtdcOrderField(Json::Value & root, const MyCThostFtdcOrderField & pField)
{
	root["Hash"] = pField._hashCode;
	root["InstrumentID"] = pField._field.InstrumentID;
	root["BrokerID"] = pField._field.BrokerID;
	root["InvestorID"] = pField._field.InvestorID;
	root["OrderRef"] = pField._field.OrderRef;
	root["UserID"] = pField._field.UserID;
	root["OrderPriceType"] = pField._field.OrderPriceType;
	root["Direction"] = pField._field.Direction;
	root["CombOffsetFlag"] = pField._field.CombOffsetFlag;
	root["CombHedgeFlag"] = pField._field.CombHedgeFlag;
	root["LimitPrice"] = pField._field.LimitPrice;
	root["VolumeTotalOriginal"] = pField._field.VolumeTotalOriginal;
	root["TimeCondition"] = pField._field.TimeCondition;
	root["VolumeCondition"] = pField._field.VolumeCondition;
	root["MinVolume"] = pField._field.MinVolume;
	root["ContingentCondition"] = pField._field.ContingentCondition;
	root["GTDDate"] = pField._field.GTDDate;
	root["StopPrice"] = pField._field.StopPrice;
	root["ForceCloseReason"] = pField._field.ForceCloseReason;
	root["IsAutoSuspend"] = pField._field.IsAutoSuspend;
	root["BusinessUnit"] = pField._field.BusinessUnit;
	root["RequestID"] = pField._field.RequestID;
	root["OrderLocalID"] = pField._field.OrderLocalID;
	root["ExchangeID"] = pField._field.ExchangeID;
	root["ParticipantID"] = pField._field.ParticipantID;
	root["ClientID"] = pField._field.ClientID;
	root["ExchangeInstID"] = pField._field.ExchangeInstID;
	root["TraderID"] = pField._field.TraderID;
	root["InstallID"] = pField._field.InstallID;
	root["OrderSubmitStatus"] = pField._field.OrderSubmitStatus;
	root["NotifySequence"] = pField._field.NotifySequence;
	root["TradingDay"] = pField._field.TradingDay;
	root["SettlementID"] = pField._field.SettlementID;
	root["OrderSysID"] = pField._field.OrderSysID;
	root["OrderSource"] = pField._field.OrderSource;
	root["OrderStatus"] = pField._field.OrderStatus;
	root["OrderType"] = pField._field.OrderType;
	root["VolumeTraded"] = pField._field.VolumeTraded;

	root["VolumeTotal"] = pField._field.VolumeTotal;
	root["InsertDate"] = pField._field.InsertDate;
	root["InsertTime"] = pField._field.InsertTime;
	root["ActiveTime"] = pField._field.ActiveTime;
	root["SuspendTime"] = pField._field.SuspendTime;
	root["UpdateTime"] = pField._field.UpdateTime;
	root["CancelTime"] = pField._field.CancelTime;
	root["ActiveTraderID"] = pField._field.ActiveTraderID;
	root["ClearingPartID"] = pField._field.ClearingPartID;
	root["SequenceNo"] = pField._field.SequenceNo;
	root["FrontID"] = pField._field.FrontID;
	root["SessionID"] = pField._field.SessionID;
	root["UserProductInfo"] = pField._field.UserProductInfo;
	root["StatusMsg"] = pField._field.StatusMsg;
	root["UserForceClose"] = pField._field.UserForceClose;
	root["BrokerOrderSeq"] = pField._field.BrokerOrderSeq;
	root["RelativeOrderSysID"] = pField._field.RelativeOrderSysID;
}

void instrument_json_helper::SerializeJsonCThostFtdcOrderField(Json::Value & value, const std::vector<MyCThostFtdcOrderField>& fields)
{
	for (size_t i = 0; i < fields.size(); ++i)
	{
		Json::Value val;
		SerializeJsonCThostFtdcOrderField(val, fields[i]);
		value.append(val);
	}
}

//void InstrumentJsonHelper::SerializeJsonCThostFtdcOrderField(Json::Value & value, const std::map<TThostFtdcPosiDirectionType, std::vector<CThostFtdcOrderField*>>& fields)
//{
//}
/***报单反序列化***/
MyCThostFtdcOrderField instrument_json_helper::DeserializeJsonCThostFtdcOrderField(const std::string &field)
{
	MyCThostFtdcOrderField pField = { 0 };
	Json::Reader parser;
	Json::Value root;
	if (parser.parse(field, root, false))
	{
		if (root.isArray()) return pField;
		return DeserializeJsonCThostFtdcOrderField(root);
	}
	return pField;
}
//反序列化
MyCThostFtdcOrderField  instrument_json_helper::DeserializeJsonCThostFtdcOrderField(Json::Value &root)
{
	MyCThostFtdcOrderField pField = { 0 };
	if (root.size() <= 0) return pField;

	strncpy_s(pField._field.InstrumentID, root["InstrumentID"].asCString(), sizeof(TThostFtdcInstrumentIDType)-1);
	strncpy_s(pField._field.BrokerID, root["BrokerID"].asCString(), sizeof(TThostFtdcBrokerIDType) - 1);
	strncpy_s(pField._field.InvestorID, root["InvestorID"].asCString(), sizeof(TThostFtdcInvestorIDType) - 1);
	strncpy_s(pField._field.OrderRef, root["OrderRef"].asCString(), sizeof(TThostFtdcOrderRefType) - 1);
	strncpy_s(pField._field.UserID, root["UserID"].asCString(), sizeof(TThostFtdcUserIDType) - 1);
	pField._field.OrderPriceType = *root["OrderPriceType"].asCString();
	pField._field.Direction = *root["Direction"].asCString();
	strncpy_s(pField._field.CombOffsetFlag, root["CombOffsetFlag"].asCString(), sizeof(TThostFtdcCombOffsetFlagType) - 1);
	strncpy_s(pField._field.CombHedgeFlag, root["CombHedgeFlag"].asCString(), sizeof(TThostFtdcCombHedgeFlagType) - 1);
	
	pField._field.LimitPrice = root["LimitPrice"].asDouble();
	pField._field.VolumeTotalOriginal = root["VolumeTotalOriginal"].asInt();
	pField._field.TimeCondition = *root["TimeCondition"].asCString();
	pField._field.VolumeCondition = *root["VolumeCondition"].asCString();
	pField._field.MinVolume = root["MinVolume"].asInt();
	pField._field.ContingentCondition = *root["ContingentCondition"].asCString();
	strncpy_s(pField._field.GTDDate, root["GTDDate"].asCString(), sizeof(TThostFtdcDateType) - 1);
	pField._field.StopPrice = root["StopPrice"].asDouble();
	pField._field.ForceCloseReason = *root["ForceCloseReason"].asCString();
	pField._field.IsAutoSuspend = root["IsAutoSuspend"].asInt();
	strncpy_s(pField._field.BusinessUnit ,root["BusinessUnit"].asCString(),sizeof(TThostFtdcBusinessUnitType));
	pField._field.RequestID = root["RequestID"].asInt();
	strncpy_s(pField._field.OrderLocalID,root["OrderLocalID"].asCString(),sizeof(TThostFtdcOrderLocalIDType)-1);
	strncpy_s(pField._field.ExchangeID,root["ExchangeID"].asCString() ,sizeof(TThostFtdcExchangeIDType)-1);
	strncpy_s(pField._field.ParticipantID,root["ParticipantID"].asCString(),sizeof(TThostFtdcParticipantIDType)-1);
	strncpy_s(pField._field.ClientID, root["ClientID"].asCString(), sizeof(TThostFtdcClientIDType) - 1);
	strncpy_s(pField._field.ExchangeInstID,root["ExchangeInstID"].asCString(), sizeof(TThostFtdcExchangeInstIDType) - 1);

	strncpy_s(pField._field.TraderID, root["TraderID"].asCString(), sizeof(TThostFtdcTraderIDType) - 1);
	strncpy_s(pField._field.TradingDay, root["TradingDay"].asCString(), sizeof(TThostFtdcDateType) - 1);
	pField._field.SettlementID = root["SettlementID"].asInt();
	strncpy_s(pField._field.OrderSysID, root["OrderSysID"].asCString(), sizeof(TThostFtdcOrderSysIDType) - 1);
	pField._field.OrderSource = *root["OrderSource"].asCString();
	pField._field.InstallID = root["InstallID"].asInt();
	pField._field.OrderSubmitStatus = *root["OrderSubmitStatus"].asCString();
	pField._field.NotifySequence = root["NotifySequence"].asInt();
	pField._field.OrderStatus = *root["OrderStatus"].asCString();
	pField._field.OrderType = *root["OrderType"].asCString();
	pField._field.VolumeTraded = root["VolumeTraded"].asInt();
	strncpy_s(pField._field.InsertDate, root["InsertDate"].asCString(), sizeof(TThostFtdcDateType) - 1);
	strncpy_s(pField._field.InsertTime, root["InsertTime"].asCString(), sizeof(TThostFtdcTimeType) - 1);
	strncpy_s(pField._field.ActiveTime, root["ActiveTime"].asCString(), sizeof(TThostFtdcTimeType) - 1);
	strncpy_s(pField._field.SuspendTime, root["SuspendTime"].asCString(), sizeof(TThostFtdcTimeType) - 1);
	strncpy_s(pField._field.UpdateTime, root["UpdateTime"].asCString(), sizeof(TThostFtdcTimeType) - 1);
	strncpy_s(pField._field.CancelTime, root["CancelTime"].asCString(), sizeof(TThostFtdcTimeType) - 1);

	strncpy_s(pField._field.ActiveTraderID, root["ActiveTraderID"].asCString(), sizeof(TThostFtdcTraderIDType) - 1);
	strncpy_s(pField._field.ClearingPartID, root["ClearingPartID"].asCString(), sizeof(TThostFtdcParticipantIDType) - 1);
	pField._field.SequenceNo = root["SequenceNo"].asInt();
	pField._field.FrontID = root["FrontID"].asInt();
	pField._field.SessionID = root["SessionID"].asInt();
	strncpy_s(pField._field.UserProductInfo, root["UserProductInfo"].asCString(), sizeof(TThostFtdcProductInfoType) - 1);
	strncpy_s(pField._field.StatusMsg, root["StatusMsg"].asCString(), sizeof(TThostFtdcErrorMsgType) - 1);
	pField._field.UserForceClose = root["UserForceClose"].asInt();
	pField._field.BrokerOrderSeq = root["BrokerOrderSeq"].asInt();
	strncpy_s(pField._field.RelativeOrderSysID, root["RelativeOrderSysID"].asCString(), sizeof(TThostFtdcOrderSysIDType) - 1);
	pField._field.VolumeTotal = root["VolumeTotal"].asInt();

	//重新计算吧，应该是一样的
	pField.RecalculateHash();
	return pField;
}
void instrument_json_helper::DeserializeJsonCThostFtdcOrderField(std::vector<MyCThostFtdcOrderField> &fields, Json::Value &value, const std::string &content)
{
	if (content.empty()) return;
	Json::Reader parser;
	Json::Value root;
	if (parser.parse(content, root, false))
	{
		if (root.isArray())
		{
			for (Json::ArrayIndex  i = 0; i < root.size(); ++i)
			{
				MyCThostFtdcOrderField pField = DeserializeJsonCThostFtdcOrderField(root[i]);
				if (strlen(pField._field.InstrumentID) != 0)
				{
					fields.push_back(pField);
				}
			}
		}
	}
}

//成交
void instrument_json_helper::SerializeJsonCThostFtdcTradeField(Json::Value & root, const CThostFtdcTradeField & pField)
{
	root["InstrumentID"] = pField.InstrumentID;
	root["BrokerID"] = pField.BrokerID;
	root["InvestorID"] = pField.InvestorID;
	root["OrderRef"] = pField.OrderRef;
	root["UserID"] = pField.UserID;
	root["ExchangeID"] = pField.ExchangeID;
	root["Direction"] = pField.Direction;
	root["OrderSysID"] = pField.OrderSysID;
	root["ParticipantID"] = pField.ParticipantID;
	root["ClearingPartID"] = pField.ClearingPartID;
	root["TradingRole"] = pField.TradingRole;
	root["OffsetFlag"] = pField.OffsetFlag;
	root["Price"] = pField.Price;
	root["HedgeFlag"] = pField.HedgeFlag;
	root["Volume"] = pField.Volume;
	root["TradeDate"] = pField.TradeDate;
	root["TradeTime"] = pField.TradeTime;
	root["TradeType"] = pField.TradeType;
	root["PriceSource"] = pField.PriceSource;
	root["TraderID"] = pField.TraderID;
	root["OrderLocalID"] = pField.OrderLocalID;
	root["ClearingPartID"] = pField.ClearingPartID;
	root["BusinessUnit"] = pField.BusinessUnit;
	root["SequenceNo"] = pField.SequenceNo;
	root["TradingDay"] = pField.TradingDay;
	root["SettlementID"] = pField.SettlementID;
	root["BrokerOrderSeq"] = pField.BrokerOrderSeq;
	root["TradeSource"] = pField.TradeSource;
	root["ExchangeInstID"] = pField.ExchangeInstID;
}

void instrument_json_helper::SerializeJsonCThostFtdcTradeField(Json::Value & value, const std::vector<CThostFtdcTradeField>& fields)
{
	if (fields.empty()) return;
	for (size_t i = 0; i < fields.size(); ++i)
	{
		Json::Value val;
		SerializeJsonCThostFtdcTradeField(val, fields[i]);
		value.append(val);
	}
}

CThostFtdcTradeField instrument_json_helper::DeserializeJsonCThostFtdcTradeField(const std::string & field)
{
	CThostFtdcTradeField pField = { 0 };
	if (field.empty()) return pField;
	Json::Reader parser;
	Json::Value root;
	if (parser.parse(field, root, false))
	{
		if (root.isArray()) return pField;
		return DeserializeJsonCThostFtdcTradeField(root);
	}
	return pField;
}

CThostFtdcTradeField instrument_json_helper::DeserializeJsonCThostFtdcTradeField(Json::Value & root)
{
	CThostFtdcTradeField pField = { 0 };
	if (root.size() <= 0) return pField;
	strncpy_s(pField.InstrumentID, root["InstrumentID"].asCString(), sizeof(TThostFtdcInstrumentIDType) - 1);
	strncpy_s(pField.BrokerID, root["BrokerID"].asCString(), sizeof(TThostFtdcBrokerIDType) - 1);
	strncpy_s(pField.InvestorID, root["InvestorID"].asCString(), sizeof(TThostFtdcInvestorIDType) - 1);
	strncpy_s(pField.OrderRef, root["OrderRef"].asCString(), sizeof(TThostFtdcOrderRefType) - 1);
	strncpy_s(pField.UserID, root["UserID"].asCString(), sizeof(TThostFtdcUserIDType) - 1);
	pField.Price = root["Price"].asDouble();
	pField.Direction = *root["Direction"].asCString();
	pField.OffsetFlag = *root["OffsetFlag"].asCString();
	pField.HedgeFlag = *root["HedgeFlag"].asCString();
	strncpy_s(pField.BusinessUnit, root["BusinessUnit"].asCString(), sizeof(TThostFtdcBusinessUnitType));
	strncpy_s(pField.OrderLocalID, root["OrderLocalID"].asCString(), sizeof(TThostFtdcOrderLocalIDType) - 1);
	strncpy_s(pField.ExchangeID, root["ExchangeID"].asCString(), sizeof(TThostFtdcExchangeIDType) - 1);
	strncpy_s(pField.ParticipantID, root["ParticipantID"].asCString(), sizeof(TThostFtdcParticipantIDType) - 1);
	strncpy_s(pField.ClientID, root["ClientID"].asCString(), sizeof(TThostFtdcClientIDType) - 1);
	strncpy_s(pField.ExchangeInstID, root["ExchangeInstID"].asCString(), sizeof(TThostFtdcExchangeInstIDType) - 1);
	strncpy_s(pField.TraderID, root["TraderID"].asCString(), sizeof(TThostFtdcTraderIDType) - 1);
	strncpy_s(pField.TradingDay, root["TradingDay"].asCString(), sizeof(TThostFtdcDateType) - 1);
	pField.SettlementID = root["SettlementID"].asInt();
	strncpy_s(pField.OrderSysID, root["OrderSysID"].asCString(), sizeof(TThostFtdcOrderSysIDType) - 1);
	strncpy_s(pField.ClearingPartID, root["ClearingPartID"].asCString(), sizeof(TThostFtdcParticipantIDType) - 1);
	pField.SequenceNo = root["SequenceNo"].asInt();
	pField.BrokerOrderSeq = root["BrokerOrderSeq"].asInt();
	pField.TradingRole = *root["TradingRole"].asCString();
	pField.Volume = root["Volume"].asInt();
	strncpy_s(pField.TradeDate,root["TradeDate"].asCString(),sizeof(TThostFtdcDateType)-1);
	strncpy_s(pField.TradeTime, root["TradeTime"].asCString(),sizeof(TThostFtdcTimeType)-1);
	pField.TradeType = *root["TradeType"].asCString();
	pField.PriceSource = *root["PriceSource"].asCString();
	pField.TradeSource = *root["TradeSource"].asCString();

	return pField;
}

void instrument_json_helper::DeserializeJsonCThostFtdcTradeField(std::vector<CThostFtdcTradeField>& fields, Json::Value & value, const std::string & content)
{
	if (content.empty()) return;
	Json::Reader parser;
	Json::Value root;
	if (parser.parse(content, root, false))
	{
		if (root.isArray())
		{
			for (Json::ArrayIndex  i = 0; i < root.size(); ++i)
			{
				CThostFtdcTradeField pField = DeserializeJsonCThostFtdcTradeField(root[i]);
				if (strlen(pField.InstrumentID) != 0)
				{
					fields.push_back(pField);
				}
			}
		}
	}
}

/*报单错误*/
void instrument_json_helper::SerializeJsonCThostFtdcInputOrderField(Json::Value &root, const CThostFtdcInputOrderField &pField)
{
	root["InstrumentID"] = pField.InstrumentID;
	root["BrokerID"] = pField.BrokerID;
	root["InvestorID"] = pField.InvestorID;
	root["OrderRef"] = pField.OrderRef;
	root["UserID"] = pField.UserID;
	root["OrderPriceType"] = pField.OrderPriceType;
	root["Direction"] = pField.Direction;
	root["CombOffsetFlag"] = pField.CombOffsetFlag;
	root["CombHedgeFlag"] = pField.CombHedgeFlag;
	root["LimitPrice"] = pField.LimitPrice;
	root["VolumeTotalOriginal"] = pField.VolumeTotalOriginal;
	root["TimeCondition"] = pField.TimeCondition;
	root["VolumeCondition"] = pField.VolumeCondition;
	root["MinVolume"] = pField.MinVolume;
	root["ContingentCondition"] = pField.ContingentCondition;
	root["GTDDate"] = pField.GTDDate;
	root["StopPrice"] = pField.StopPrice;
	root["ForceCloseReason"] = pField.ForceCloseReason;
	root["IsAutoSuspend"] = pField.IsAutoSuspend;
	root["BusinessUnit"] = pField.BusinessUnit;
	root["RequestID"] = pField.RequestID;
	root["UserForceClose"] = pField.UserForceClose;	
}
void instrument_json_helper::SerializeJsonCThostFtdcInputOrderField(Json::Value &value, const std::vector<CThostFtdcInputOrderField> &fields)
{
	if (fields.empty()) return;
	for (size_t i = 0; i < fields.size(); ++i)
	{
		Json::Value val;
		SerializeJsonCThostFtdcInputOrderField(val, fields[i]);
		value.append(val);
	}
}
/*deserialize报单错误*/
CThostFtdcInputOrderField instrument_json_helper::DeserializeJsonCThostFtdcInputOrderField(const std::string &field)
{
	CThostFtdcInputOrderField pField = { 0 };
	if (field.empty()) return pField;
	Json::Reader parser;
	Json::Value root;
	if (parser.parse(field, root, false))
	{
		if (root.isArray()) return pField;
		return DeserializeJsonCThostFtdcInputOrderField(root);
	}
	return pField;
}
CThostFtdcInputOrderField instrument_json_helper::DeserializeJsonCThostFtdcInputOrderField(Json::Value &root)
{
	CThostFtdcInputOrderField pField = { 0 };
	if (root.size() <= 0) return pField;

	strncpy_s(pField.InstrumentID, root["InstrumentID"].asCString(), sizeof(TThostFtdcInstrumentIDType) - 1);
	strncpy_s(pField.BrokerID, root["BrokerID"].asCString(), sizeof(TThostFtdcBrokerIDType) - 1);
	strncpy_s(pField.InvestorID, root["InvestorID"].asCString(), sizeof(TThostFtdcInvestorIDType) - 1);
	strncpy_s(pField.OrderRef, root["OrderRef"].asCString(), sizeof(TThostFtdcOrderRefType) - 1);
	strncpy_s(pField.UserID, root["UserID"].asCString(), sizeof(TThostFtdcUserIDType) - 1);
	pField.OrderPriceType = *root["OrderPriceType"].asCString();
	pField.Direction = *root["Direction"].asCString();
	strncpy_s(pField.CombOffsetFlag, root["CombOffsetFlag"].asCString(), sizeof(TThostFtdcCombOffsetFlagType) - 1);
	strncpy_s(pField.CombHedgeFlag, root["CombHedgeFlag"].asCString(), sizeof(TThostFtdcCombHedgeFlagType) - 1);

	pField.LimitPrice = root["LimitPrice"].asDouble();
	pField.VolumeTotalOriginal = root["VolumeTotalOriginal"].asInt();
	pField.TimeCondition = *root["TimeCondition"].asCString();
	pField.VolumeCondition = *root["VolumeCondition"].asCString();
	pField.MinVolume = root["MinVolume"].asInt();
	pField.ContingentCondition = *root["ContingentCondition"].asCString();
	strncpy_s(pField.GTDDate, root["GTDDate"].asCString(), sizeof(TThostFtdcDateType) - 1);
	pField.StopPrice = root["StopPrice"].asDouble();
	pField.ForceCloseReason = *root["ForceCloseReason"].asCString();
	pField.IsAutoSuspend = root["IsAutoSuspend"].asInt();
	strncpy_s(pField.BusinessUnit, root["BusinessUnit"].asCString(), sizeof(TThostFtdcBusinessUnitType));
	pField.RequestID = root["RequestID"].asInt();
	pField.UserForceClose = root["UserForceClose"].asInt();

	return pField;
}
void instrument_json_helper::DeserializeJsonCThostFtdcInputOrderField(std::vector<CThostFtdcInputOrderField > &fields, Json::Value &value, const std::string &content)
{
	if (content.empty()) return;
	Json::Reader parser;
	Json::Value root;
	if (parser.parse(content, root, false))
	{
		if (root.isArray())
		{
			for (Json::ArrayIndex  i = 0; i < root.size(); ++i)
			{
				CThostFtdcInputOrderField pField = DeserializeJsonCThostFtdcInputOrderField(root[i]);
				if (strlen(pField.InstrumentID) != 0)
				{
					fields.push_back(pField);
				}
			}
		}
	}
}

void instrument_json_helper::SerializeJsonInstrumentRuntime(Json::Value &root, const InstrumentRuntime & instrumentruntime)
{
	// 按照顺序写上，如果空，说明没有，否则不能解析
	Json::Value positions(Json::ValueType::arrayValue);
	SerializeJsonCThostFtdcInvestorPositionField(positions, instrumentruntime.investorPositionAllDirections);
	root.append(positions);

	Json::Value ftdcOrderFields(Json::ValueType::arrayValue);
	SerializeJsonCThostFtdcOrderField(ftdcOrderFields, instrumentruntime.ftdcOrderFields);
	root.append(ftdcOrderFields);

	Json::Value ftdcTradeFields(Json::ValueType::arrayValue);
	SerializeJsonCThostFtdcTradeField(ftdcTradeFields, instrumentruntime.ftdcTradeFields);
	root.append(ftdcOrderFields);

	Json::Value ftdcInputOrderFields(Json::ValueType::arrayValue);
	SerializeJsonCThostFtdcInputOrderField(ftdcInputOrderFields, instrumentruntime.ftdcInputOrderFields);
	root.append(ftdcInputOrderFields);
}