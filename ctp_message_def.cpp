#include "ctp_message_def.h"


const char *ctp_message_def::KEY_ACCOUNT_INFO_SUFFIX = "_AccountInfo";

const char *ctp_message_def::KEY_ACCOUNT_LOGIN_INFO_SUFFIX = "_AccountLoginInfo";

const char *ctp_message_def::KEY_ACCOUNT_INVESTORPOSITIONFIELD_SUFFIX = "_InvestorPositionFieldInfo_";

const char *ctp_message_def::KEY_ACCOUNT_CTHOSTFTDCORDERFIELD_SUFFIX = "_CThostFtdcOrderFieldInfo_"; 

const char *ctp_message_def::KEY_ACCOUNT_PENDING_CTHOSTFTDCORDERFIELD_SUFFIX = "_Pending_CThostFtdcOrderFieldInfo_";

const char *ctp_message_def::KEY_ACCOUNT_CTHOSTFTDCTRADEFIELD_SUFFIX = "_CThostFtdcTradeFieldInfo_"; 

const char *ctp_message_def::KEY_ACCOUNT_CTHOSTFTDCINPUTORDERFIELD_SUFFIX = "_CThostFtdcInputOrderFieldInfo_";

//这个同时当做key和event
const char *ctp_message_def::KEY_CTP_INSTRUMENT_IDS = "ctp_instrument_ids";

const char * ctp_message_def::KEY_CTP_INSTRUMENT_VALUE_SIMPLE_POSTFIX = "_ins_value_s";

const char * ctp_message_def::CHANNEL_INSTRUMENT_VALUE_SIMPLE = "instrument_value_s";

const char * ctp_message_def::LOCAL_STRATEGIES_SUFFIX = "ctp_strategies_";

const char * ctp_message_def::EVENT_SUFFIX = "event_";

const char * ctp_message_def::SUBSCRIBE_MARKET_DATA="quote_subscribe_market_data";

const char * ctp_message_def::UNSUBSCRIBE_MARKET_DATA= "quote_unsubscribe_market_data";

const char * ctp_message_def::INVESTOR_STRATEGIES_STOP = "investor_strategies_stop_";

const char* ctp_message_def::INVESTOR_TRADEAPP_STOP = "investor_tradeapp_stop_";

//RB1805_kline_value_5，5分钟k线,这个地方用的不是ctp的品种代码
//RB1805_kline_value_30，30分钟k线
const char* ctp_message_def::KEY_KLINE_REDIS_VALUE = "_kline_value_";


const char* ctp_message_def::EVENT_KLINE_VALUE_DONE= "event_kline_value_done";
ctp_message_def::ctp_message_def()
{
}


ctp_message_def::~ctp_message_def()
{
}
