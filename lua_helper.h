#ifndef LUA_HELPER_H__
#define LUA_HELPER_H__
#include <string>
/*
lua ���ýű�
*/
class lua_helper
{
public:
	lua_helper();
	~lua_helper();

	/*
	��������� �ű��ļ� ��Ӧ���ڵĿ��� ��� ��� ����ֵ
	��������� �ýű�Ӧ�õĶ൥�Ϳյ�����
	*/
	static void invoke_script(const char *filename, 
		bool finished, 
		float open,
		float high, 
		float low, 
		float close, 
		const char *trade_date,
		int &buyCnt, 
		int&sellCnt);

	static void invoke_script(const std::string &content,
		bool finished,
		float open,
		float high,
		float low,
		float close,
		const char *trade_date,
		int &buyCnt,
		int&sellCnt);
};

#endif // !LUA_HELPER_H__