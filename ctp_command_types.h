#ifndef CTP_COMMAND_TYPES_H__
#define CTP_COMMAND_TYPES_H__
#include<string>
#include <vector>
typedef enum CTP_COMMAND_TYPE
{
	CTP_COMMAND_NULL = 0,
	CTP_COMMAND_CLOSE_ALL,
	CTP_COMMAND_CLOSE,
	CTP_COMMAND_OPEN,
	CTP_COMMAND_LIST_INVESTOR_POSITIONS,//����Ķ���һ���Ե������������Щ�־û�������(�ͽб����������)
	CTP_COMMAND_ENSURE_INS_DIR_NUMBERS,//��������ĳ����Լ��ĳ�����򿪵����������˾�ƽ�����˾Ϳ�����ƽ�����Լ���������ʱ���������ɹ���
	CTP_COMMAND_LOSS_LIMIT,//ֹ�𣬿���ĳ����ߺܶ���
	CTP_COMMAND_WIN_LIMIT,//ֹӯ
	CTP_COMMAND_RERESH_INS,
	CTP_COMMAND_CANCEL_ORDER,
}CTP_COMMAND_TYPE;


typedef enum CTP_COMMAND_ARGU_DIRECTION
{
	BUY_DIRECTION=1,
	SELL_DIRECTION=-1
}CTP_COMMAND_ARGU_DIRECTION;

class ctp_command_argus
{
public:
	ctp_command_argus():_instrumentId(""), _direction(CTP_COMMAND_ARGU_DIRECTION::SELL_DIRECTION),_price(0.0),_number(0), _enabled(true)
	{
	}
public:
	// ��Լ
	std::string _instrumentId;
	// ����,����ط���ԭʼ���򣬱�ʾ�൥���ǿյ�
	CTP_COMMAND_ARGU_DIRECTION _direction;
	// �۸�
	double _price;

	// ����
	int _number;

	//
	bool _enabled;

	//cancel 
	int _sessionID;
	int _frontID;
	std::string _orderRef;
	// hash
	std::vector<unsigned int> _order_hash;
};

class ctp_command {
public:
	ctp_command():_type(CTP_COMMAND_TYPE::CTP_COMMAND_NULL){}
public:
	// command
	CTP_COMMAND_TYPE _type;

	// all argus
	std::vector<ctp_command_argus> _allArgus;

	bool empty() { return _allArgus.empty(); }
};
#endif // !CTP_COMMAND_TYPES_H__

