#ifndef CTP_MESSAGE_DEF_H__
#define CTP_MESSAGE_DEF_H__

//作为本地服务应用，貌似redis够用了，先不用socket服务器了
class ctp_message_def
{
public:
	// 0100 0001: 登录信息
	// 登录信息包括如下：
	// 初始登录时间
	// 刷新时间->定时刷新，超过一定时间认定断开登录
	// 登录终端id，每台机器可以分配一个id，系统配置每个机器允许登录多少个
	// 资金状况
	// 登出时间，如果登出了
	static const char TAG_CTP_SESSION_INFO = 0X41;

	//持仓,所有当前持仓信息，json格式
	// 更新终端id，哪个终端更新的
	// 更新时间，最近的更新时间
	static const char TAG_CTP_CTHOSTFTDCINVESTORPOSITIONFIELD = 0x42;

	//报单，所有当前报单信息，json格式
	// 更新终端id，哪个终端更新的
	// 更新时间，最近的更新时间
	static const char TAG_CTP_CTHOSTFTDCORDERFIELD = 0x43;

	//成交记录
	// 更新终端id，哪个终端更新的
	// 更新时间，最近的更新时间
	static const char TAG_CTP_CTHOSTFTDCTRADEFIELD = 0x44;

	// 账户信息 后缀
	static const char *KEY_ACCOUNT_INFO_SUFFIX;
	// 账户登录信息后缀
	static const char *KEY_ACCOUNT_LOGIN_INFO_SUFFIX;
	// 持仓后缀
	static const char *KEY_ACCOUNT_INVESTORPOSITIONFIELD_SUFFIX;
	//所有报单
	static const char *KEY_ACCOUNT_CTHOSTFTDCORDERFIELD_SUFFIX;

	//pending报单
	static const char *KEY_ACCOUNT_PENDING_CTHOSTFTDCORDERFIELD_SUFFIX;
	//成交记录
	static const char *KEY_ACCOUNT_CTHOSTFTDCTRADEFIELD_SUFFIX;

	//报单错误
	static const char * KEY_ACCOUNT_CTHOSTFTDCINPUTORDERFIELD_SUFFIX;

	// collection of instrument ids
	static const char * KEY_CTP_INSTRUMENT_IDS;

	// 更新instrment simple value, 然后同步更新事件也用这个
	// 比如说 subscribe("rb1705_"+KEY_CTP_INSTRUMENT_VALUE_SIMPLE_POSTFIX,....)
	// 为了效率考虑？，先阶段只支持每个品种单独注册行情回调
	static const char * KEY_CTP_INSTRUMENT_VALUE_SIMPLE_POSTFIX;

	// 这个是所有的品种注册一个行情，那么所有的品种的变更都会收到
	static const char * CHANNEL_INSTRUMENT_VALUE_SIMPLE;

	//策略前缀
	static const char * LOCAL_STRATEGIES_SUFFIX;

	// 策略更新事件
	static const char * EVENT_SUFFIX;

	// 注册行情,注销行情
	static const char * SUBSCRIBE_MARKET_DATA;
	static const char * UNSUBSCRIBE_MARKET_DATA;

	// 策略停止事件
	static const char * INVESTOR_STRATEGIES_STOP;

	//交易程序停止事件
	static const char * INVESTOR_TRADEAPP_STOP;


	//k线的redis key
	static const char * KEY_KLINE_REDIS_VALUE;

	//当前正在进行的k线的时间戳
	// 如果这个变了的花，就去获取上面的值，合并k线，然后执行策略。
	static const char * EVENT_KLINE_VALUE_DONE;
public:
	ctp_message_def();
	~ctp_message_def();
};

#endif // !CTP_MESSAGE_DEF_H__