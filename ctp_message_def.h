#ifndef CTP_MESSAGE_DEF_H__
#define CTP_MESSAGE_DEF_H__

//��Ϊ���ط���Ӧ�ã�ò��redis�����ˣ��Ȳ���socket��������
class ctp_message_def
{
public:
	// 0100 0001: ��¼��Ϣ
	// ��¼��Ϣ�������£�
	// ��ʼ��¼ʱ��
	// ˢ��ʱ��->��ʱˢ�£�����һ��ʱ���϶��Ͽ���¼
	// ��¼�ն�id��ÿ̨�������Է���һ��id��ϵͳ����ÿ�����������¼���ٸ�
	// �ʽ�״��
	// �ǳ�ʱ�䣬����ǳ���
	static const char TAG_CTP_SESSION_INFO = 0X41;

	//�ֲ�,���е�ǰ�ֲ���Ϣ��json��ʽ
	// �����ն�id���ĸ��ն˸��µ�
	// ����ʱ�䣬����ĸ���ʱ��
	static const char TAG_CTP_CTHOSTFTDCINVESTORPOSITIONFIELD = 0x42;

	//���������е�ǰ������Ϣ��json��ʽ
	// �����ն�id���ĸ��ն˸��µ�
	// ����ʱ�䣬����ĸ���ʱ��
	static const char TAG_CTP_CTHOSTFTDCORDERFIELD = 0x43;

	//�ɽ���¼
	// �����ն�id���ĸ��ն˸��µ�
	// ����ʱ�䣬����ĸ���ʱ��
	static const char TAG_CTP_CTHOSTFTDCTRADEFIELD = 0x44;

	// �˻���Ϣ ��׺
	static const char *KEY_ACCOUNT_INFO_SUFFIX;
	// �˻���¼��Ϣ��׺
	static const char *KEY_ACCOUNT_LOGIN_INFO_SUFFIX;
	// �ֲֺ�׺
	static const char *KEY_ACCOUNT_INVESTORPOSITIONFIELD_SUFFIX;
	//���б���
	static const char *KEY_ACCOUNT_CTHOSTFTDCORDERFIELD_SUFFIX;

	//pending����
	static const char *KEY_ACCOUNT_PENDING_CTHOSTFTDCORDERFIELD_SUFFIX;
	//�ɽ���¼
	static const char *KEY_ACCOUNT_CTHOSTFTDCTRADEFIELD_SUFFIX;

	//��������
	static const char * KEY_ACCOUNT_CTHOSTFTDCINPUTORDERFIELD_SUFFIX;

	// collection of instrument ids
	static const char * KEY_CTP_INSTRUMENT_IDS;

	// ����instrment simple value, Ȼ��ͬ�������¼�Ҳ�����
	// ����˵ subscribe("rb1705_"+KEY_CTP_INSTRUMENT_VALUE_SIMPLE_POSTFIX,....)
	// Ϊ��Ч�ʿ��ǣ����Ƚ׶�ֻ֧��ÿ��Ʒ�ֵ���ע������ص�
	static const char * KEY_CTP_INSTRUMENT_VALUE_SIMPLE_POSTFIX;

	// ��������е�Ʒ��ע��һ�����飬��ô���е�Ʒ�ֵı�������յ�
	static const char * CHANNEL_INSTRUMENT_VALUE_SIMPLE;

	//����ǰ׺
	static const char * LOCAL_STRATEGIES_SUFFIX;

	// ���Ը����¼�
	static const char * EVENT_SUFFIX;

	// ע������,ע������
	static const char * SUBSCRIBE_MARKET_DATA;
	static const char * UNSUBSCRIBE_MARKET_DATA;

	// ����ֹͣ�¼�
	static const char * INVESTOR_STRATEGIES_STOP;

	//���׳���ֹͣ�¼�
	static const char * INVESTOR_TRADEAPP_STOP;


	//k�ߵ�redis key
	static const char * KEY_KLINE_REDIS_VALUE;

	//��ǰ���ڽ��е�k�ߵ�ʱ���
	// ���������˵Ļ�����ȥ��ȡ�����ֵ���ϲ�k�ߣ�Ȼ��ִ�в��ԡ�
	static const char * EVENT_KLINE_VALUE_DONE;
public:
	ctp_message_def();
	~ctp_message_def();
};

#endif // !CTP_MESSAGE_DEF_H__