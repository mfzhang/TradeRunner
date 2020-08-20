#ifndef MY_UTILS_H__
#define MY_UTILS_H__
#include <string>
#include <map>

class item_code_class {
public:
	std::string name;
	std::string date;
};
class my_utils
{
public:
	my_utils();
	~my_utils();

	/**constant*/
	static const char* K_USERNAME;
	static const char* K_PASSWORD;
	static const char* K_ADDRESS;
	static const char* K_BROKER;
	static const char* K_REDIS_ADDRESS;
	static const char* K_REDISPORT;
	static const char* K_MANIFEST_FILE_NAME;
	static const char* K_STRATEGY_SCRIPT_EXTENSION;
	static const char* K_WODEQUANT_FOLDER;
	static const char* K_STRATEGY_PARAMETER_FILE_EXTENSION;

	//mysql数据库连接接参数
	static const char* K_DB_STRING;
	static const char* K_DB_USERNAME;
	static const char* K_DB_PWD;
	// qita 参数
	static const char* K_MODE_ONLY_REFRESH_STRATETIES;
	/*static method**/
	static std::string &get_local_ip_address();
	//读取文件内容，处理中文字符
	static std::string get_file_content(const char* filename);
	//字符串替换
	static std::string&  replace_all_distinct(std::string&   str, const   std::string&   old_value, const   std::string&   new_value);
	//得到当前时间
	static time_t get_local_time();
	static const std::string &get_current_guid();
	static unsigned int hash(const char *str)
	{
		register unsigned int h;
		register unsigned char *p;

		for (h = 0, p = (unsigned char *)str; *p; p++)
			h = 31 * h + *p;
		return h;
	}

	static const item_code_class &get_item_code(const char * _code);
	/*********************/
private:
	static std::string local_ip_address;
	static std::string current_guid;

	static std::map<std::string, item_code_class> ctpcode2itemcode;
};

#endif // !MY_UTILS_H__