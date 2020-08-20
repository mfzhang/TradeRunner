#ifndef LUA_HELPER_H__
#define LUA_HELPER_H__
#include <string>
/*
lua 调用脚本
*/
class lua_helper
{
public:
	lua_helper();
	~lua_helper();

	/*
	传入参数： 脚本文件 相应周期的开盘 最高 最低 收盘值
	输出参数： 该脚本应该的多单和空单数量
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