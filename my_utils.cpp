#include "my_utils.h"
#include <boost/asio.hpp> 
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <fstream>
#include <windows.h>

using namespace boost::uuids;

const  char* my_utils::K_USERNAME = "username";
const  char* my_utils::K_PASSWORD = "password";
const  char* my_utils::K_ADDRESS = "address";
const  char* my_utils::K_BROKER = "broker";
const  char* my_utils::K_REDIS_ADDRESS = "redis_address";
const  char* my_utils::K_REDISPORT = "redis_port";
const  char* my_utils::K_MANIFEST_FILE_NAME = "strategy_manifest.wode";
const  char* my_utils::K_STRATEGY_SCRIPT_EXTENSION = ".strategy";
const  char* my_utils::K_WODEQUANT_FOLDER = "C:\\wodequant\\";
const  char* my_utils::K_STRATEGY_PARAMETER_FILE_EXTENSION = ".param";
const  char* my_utils::K_MODE_ONLY_REFRESH_STRATETIES = "refresh_strategy";

const char* my_utils::K_DB_STRING="db_string";
const char* my_utils::K_DB_USERNAME = "db_user";
const char* my_utils::K_DB_PWD="db_pwd";

std::map<std::string, item_code_class> my_utils::ctpcode2itemcode;
my_utils::my_utils()
{
}


my_utils::~my_utils()
{
}


std::string my_utils::local_ip_address;
std::string &my_utils::get_local_ip_address()
{
	// TODO: 在此处插入 return 语句

	if (local_ip_address.empty())
	{
		boost::asio::io_service io_service;
		boost::asio::ip::tcp::resolver resolver(io_service);
		boost::asio::ip::tcp::resolver::query query(boost::asio::ip::host_name(), "");
		boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
		boost::asio::ip::tcp::resolver::iterator end; // End marker.  
		while (iter != end)
		{
			boost::asio::ip::tcp::endpoint ep = *iter++;
			if (ep.address().is_v4())
			{
				local_ip_address = ep.address().to_string();
				break;
			}
		}
	}
	return local_ip_address;
}
std::string UTF8ToGB(const char* str)
{
	std::string result;
	WCHAR *strSrc;
	LPSTR szRes;

	//获得临时变量的大小
	int i = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	strSrc = new WCHAR[i + 1];
	MultiByteToWideChar(CP_UTF8, 0, str, -1, strSrc, i);

	//获得临时变量的大小
	i = WideCharToMultiByte(CP_ACP, 0, strSrc, -1, NULL, 0, NULL, NULL);
	szRes = new CHAR[i + 1];
	WideCharToMultiByte(CP_ACP, 0, strSrc, -1, szRes, i, NULL, NULL);

	result = szRes;
	delete[]strSrc;
	delete[]szRes;

	return result;
}
std::string my_utils::get_file_content(const char* filename)
{

	std::ifstream file(filename);
	if (!file.is_open())
	{
		std::cout << "Error opening file" << std::endl;
		return "";
	}
	std::string ret = "";
	std::stringstream txt;
	txt << file.rdbuf();
	file.close();
	return UTF8ToGB(txt.str().c_str());
}

std::string&  my_utils::replace_all_distinct(std::string&   str, const   std::string&   old_value, const   std::string&   new_value)
{
	for (std::string::size_type pos(0); pos != std::string::npos; pos += new_value.length()) {
		if ((pos = str.find(old_value, pos)) != std::string::npos)
			str.replace(pos, old_value.length(), new_value);
		else   break;
	}
	return   str;
}

time_t my_utils::get_local_time()
{
	tm now = to_tm(boost::posix_time::microsec_clock::local_time());
	time_t t = mktime(&now);
	return t;
}

std::string my_utils::current_guid;
const std::string &my_utils::get_current_guid()
{
	if (current_guid.empty()) {
		random_generator rgen;//随机生成器
		uuid u = rgen();//生成一个随机的UUID
		current_guid = boost::lexical_cast<std::string>(u);
	}

	return current_guid;
}

const item_code_class &my_utils::get_item_code(const char * _code)
{

	if (_code == nullptr)
	{
		item_code_class ic;
		ctpcode2itemcode[""] = ic;
		return ctpcode2itemcode[""];
	}

	if (ctpcode2itemcode.find(_code) != ctpcode2itemcode.end())
		return ctpcode2itemcode[_code];
	
	item_code_class ic;

	const char *p = _code;
	while ((*p) != 0) {
		if (*p >= '0' && *p <= '9') {
			ic.date.push_back(*p);
		}
		else if(*p >= 'a' && *p <= 'z') {
			ic.name.push_back(*p+'A'-'a');//转大写
        }
        else if ((*p >= 'A' && *p <= 'Z')){
            ic.name.push_back(*p);//转大写
        }
		++p;
	}

	if (ic.date.length() == 3) {
		ic.date.insert(ic.date.begin(), '1');
	}
	
	

	ctpcode2itemcode[_code] = ic;

	return ctpcode2itemcode[_code];
}