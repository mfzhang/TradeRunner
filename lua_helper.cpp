#include "lua_helper.h"
#define ELPP_THREAD_SAFE  
#include "easylogging++.h"
//#include "HistorySqliteDB.h"
#include "HistoryMysqlDb.h"
#include "HistoryItem.h"
#include "my_utils.h"
#include "json\json.h"
#include <boost\date_time\posix_time\posix_time.hpp>
#include <boost\date_time.hpp>
#include "strategy_context.h"
#include <boost\format.hpp>
extern "C"
{
#include "lua.h"  
#include "lualib.h"  
#include "lauxlib.h"  
};


static const std::string K_PARAMETER_FOLDER = std::string(my_utils::K_WODEQUANT_FOLDER) + "parameters\\";
// 第一版 所有给lua脚本的接口都实现在这里吧
static int get_data_of_days_before(lua_State *L)
{
	// pop得到参数
	string item = lua_tostring(L, 1);
	if (item.empty())
	{
		LOG(ERROR) << "get_data_of_days_before instrument id empty";
		return 0;
	}
	int n_days_before = lua_tointeger(L, 2);

	/*lua_pushinteger(L, 100 + i);
	lua_pushinteger(L, 200 + i);
	lua_pushinteger(L, 300 + i);
	lua_pushinteger(L, 400 + i);
*/

	//HistorySqliteDB& hsd = HistorySqliteDB::instance(item);
	HistoryMysqlDb &hsd = strategy_context::get_mutable_instance().get_history_mysql_db() ;

	// 第二个参数是分钟，一天就是24*60分钟
	std::vector<HistoryItem*> &histories = hsd.getHistoryItems(item, 24 * 60);

	// 空结果校验
	if (histories.empty())
	{
		LOG(ERROR) << item << " history day data empty";
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		return 4;
	}

	// 长度校验
	if (n_days_before > histories.size())
	{
		LOG(ERROR) << item << "history size = " << histories.size() << " when query data " << n_days_before << " days ago";
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		return 4;
	}

	HistoryItem *hItem = histories[histories.size() - n_days_before];
	lua_pushnumber(L, (hItem->m_OpenPrice));
	lua_pushnumber(L, (hItem->m_HighPrice));
	lua_pushnumber(L, (hItem->m_LowPrice));
	lua_pushnumber(L, (hItem->m_ClosePrice));

	return 4;
}

// 第一版 所有给lua脚本的接口都实现在这里吧
static int get_data_of_index(lua_State *L)
{
	// pop得到参数
	string item = lua_tostring(L, 1);
	if (item.empty())
	{
		LOG(ERROR) << "get_data_of_index instrument id empty";
		return 0;
	}
	// pop得到参数,第二个参数是周期，整数，分钟的倍数,日线就是24*60
	int cycle = lua_tointeger(L, 2);
	if (cycle <= 0)
	{
		LOG(ERROR) << "get_data_of_index cytle zero";
		lua_pushnumber(L, -1); //-1表示没有找到，外围程序自己处理当天的
		return 1;
	}

	int index = lua_tointeger(L, 3);
	if (index < 0)
	{
		LOG(ERROR) << "get_data_of_index index less than zero";
		lua_pushnumber(L, -1); //-1表示没有找到，外围程序自己处理当天的
		return 1;
	}

	/*lua_pushinteger(L, 100 + i);
	lua_pushinteger(L, 200 + i);
	lua_pushinteger(L, 300 + i);
	lua_pushinteger(L, 400 + i);
	*/

	HistoryMysqlDb &hsd = strategy_context::get_mutable_instance().get_history_mysql_db();

	// 第二个参数是分钟，一天就是24*60分钟
	std::vector<HistoryItem*> &histories = hsd.getHistoryItems(item, cycle);

	// 空结果校验
	if (histories.empty())
	{
		LOG(ERROR) << item << " history day data empty";
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		return 4;
	}

	// 长度校验
	if (index > histories.size())
	{
		LOG(ERROR) << item << "history size = " << histories.size() << " when query data " << index << " days ago";
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		return 4;
	}

	HistoryItem *hItem = histories[index];
	lua_pushnumber(L, (hItem->m_OpenPrice));
	lua_pushnumber(L, (hItem->m_HighPrice));
	lua_pushnumber(L, (hItem->m_LowPrice));
	lua_pushnumber(L, (hItem->m_ClosePrice));

	return 4;
}

/*
该函数根据传入的日期(YYYYMMDD)和合约种类，得到该日期在历史中距离今天的天数
比如说，传入昨天的日期返回1
*/
static int get_trade_date_distance(lua_State *L)
{
	// pop得到参数
	string trade_date = lua_tostring(L, 1);
	if (trade_date.empty())
	{
		LOG(ERROR) << "get_trade_date_distance trade date empty";
		lua_pushnumber(L, -1); //-1表示没有找到，外围程序自己处理当天的
		return 1;
	}

	string instrumentid = lua_tostring(L, 2);
	if (instrumentid.empty())
	{
		LOG(ERROR) << "get_trade_date_distance instrumentid empty";
		lua_pushnumber(L, -1); //-1表示没有找到，外围程序自己处理当天的
		return 1;
	}

	HistoryMysqlDb &hsd = strategy_context::get_mutable_instance().get_history_mysql_db();

	// 第二个参数是分钟，一天就是24*60分钟
	std::vector<HistoryItem*> &histories = hsd.getHistoryItems(instrumentid, 24 * 60);

	// 空结果校验，认为无穷大
	if (histories.empty())
	{
		LOG(ERROR) << "get_trade_date_distance item history empty";
		lua_pushnumber(L, -1); //-1表示没有找到，外围程序自己处理当天的
		return 1;
	}

	for (int i = histories.size() - 1; i >= 0; --i)
	{
		std::string strTime = boost::gregorian::to_iso_string(histories[i]->m_StartTime.date());
		if (strTime.compare(trade_date) == 0)
		{
			lua_pushnumber(L, histories.size() - i);
			return 1;
		}
	}

	LOG(ERROR) << "get_trade_date_distance not found ";
	lua_pushnumber(L,  -1);//-1表示没有找到，外围程序自己处理当天的
	return 1;
}


/*
该函数根据传入的时间(YYYYMMDD hhmmss),和k线周期和合约种类，得到该日期在历史中距离今天的天数
比如说，传入昨天的日期返回1
*/
static int get_trade_time_distance(lua_State *L)
{
    

    string instrumentid = lua_tostring(L, 1);
    if (instrumentid.empty())
    {
        LOG(ERROR) << "get_trade_time_distance instrumentid empty";
        lua_pushnumber(L, -1); //-1表示没有找到，外围程序自己处理当天的
        return 1;
    }

    // pop得到参数,第二个参数是周期，整数，分钟的倍数,日线就是24*60
    int cycle = lua_tointeger(L, 2);
    if (cycle == 0)
    {
        LOG(ERROR) << "get_trade_time_distance cycle zero";
        lua_pushnumber(L, -1); //-1表示没有找到，外围程序自己处理当天的
        return 1;
    }

    // pop得到参数
    string trade_date = lua_tostring(L, 3);
    if (trade_date.empty())
    {
        LOG(ERROR) << "get_trade_time_distance trade time empty";
        lua_pushnumber(L, -1); //-1表示没有找到，外围程序自己处理当天的
        return 1;
    }

    HistoryMysqlDb &hsd = strategy_context::get_mutable_instance().get_history_mysql_db();

    // 第二个参数是分钟，一天就是24*60分钟
    std::vector<HistoryItem*> &histories = hsd.getHistoryItems(instrumentid, cycle);

    // 空结果校验，认为无穷大
    if (histories.empty())
    {
        LOG(ERROR) << "get_trade_time_distance item history empty";
        lua_pushnumber(L, -1); //-1表示没有找到，外围程序自己处理当天的
        return 1;
    }

    for (int i = histories.size() - 1; i >= 0; --i)
    {
        boost::posix_time::ptime &tm = histories[i]->m_StartTime;
        string strTime = str(boost::format("%1%-%2%-%2% %4%:%5%:%6%")
            % tm.date().year()
            % tm.date().month()
            % tm.date().day()
            % tm.time_of_day().hours()
            % tm.time_of_day().minutes()
            % tm.time_of_day().seconds());        
        if (strTime.compare(trade_date) == 0)
        {
            //找到了返回
            lua_pushnumber(L, histories.size() - i);
            return 1;
        }
    }

    LOG(ERROR) << "get_trade_time_distance not found ";
    lua_pushnumber(L, -1);//-1表示没有找到，外围程序自己处理当天的
    return 1;
}

//根据品种和索引得到时间字符串
static int get_history_data_datetime(lua_State *L) 
{
    // pop得到参数,第一个参数是品种
    string instrumentid = lua_tostring(L, 1);
    if (instrumentid.empty())
    {
        LOG(ERROR) << "get_history_data_datetime instrumentid empty";
        lua_pushnumber(L, -1); //-1表示没有找到，外围程序自己处理当天的
        return 1;
    }
    // pop得到参数,第二个参数是周期，整数，分钟的倍数,日线就是24*60
    int cycle = lua_tointeger(L, 2);
    if (cycle == 0)
    {
        LOG(ERROR) << "get_history_data_datetime cytle zero";
        lua_pushnumber(L, -1); //-1表示没有找到，外围程序自己处理当天的
        return 1;
    }


    // pop得到参数,第4个是历史数据索引
    int history_index = lua_tointeger(L, 3);
    if (history_index < 0)
    {
        LOG(ERROR) << "get_history_data_datetime history_index minus value.";
        lua_pushnumber(L, -1); //-1表示没有找到，外围程序自己处理当天的
        return 1;
    }

    HistoryMysqlDb &hsd = strategy_context::get_mutable_instance().get_history_mysql_db();

    // 第二个参数是分钟，一天就是24*60分钟
    std::vector<HistoryItem*> &histories = hsd.getHistoryItems(instrumentid, cycle);

    if (history_index >= histories.size())
    {
        LOG(ERROR) << "get_history_data_datetime history_index exceed.";
        lua_pushstring(L, ""); //-1表示没有找到，外围程序自己处理当天的
        return 1;
    }
    boost::posix_time::ptime &tm = histories[history_index]->m_StartTime;
    string ret = str(boost::format("%1%-%2%-%2% %4%:%5%:%6%")
                            %tm.date().year()
                            %tm.date().month()
                            %tm.date().day()
                            %tm.time_of_day().hours()
                            % tm.time_of_day().minutes()
                            % tm.time_of_day().seconds());

    lua_pushstring(L, ret.c_str());
    return 1;
}
// 得到某个品种某个周期上的历史数据的长度
static int get_histories_count(lua_State *L)
{
    // pop得到参数,第一个参数是品种
    string instrumentid = lua_tostring(L, 1);
    if (instrumentid.empty())
    {
        LOG(ERROR) << "get_histories_count instrumentid empty";
        lua_pushnumber(L, -1); //-1表示没有找到，外围程序自己处理当天的
        return 1;
    }
    // pop得到参数,第二个参数是周期，整数，分钟的倍数,日线就是24*60
    int cycle = lua_tointeger(L, 2);
    if (cycle == 0)
    {
        LOG(ERROR) << "get_histories_count cycle zero";
        lua_pushnumber(L, -1); //-1表示没有找到，外围程序自己处理当天的
        return 1;
    }

    HistoryMysqlDb &hsd = strategy_context::get_mutable_instance().get_history_mysql_db();

    // 第二个参数是分钟，一天就是24*60分钟
    std::vector<HistoryItem*> &histories = hsd.getHistoryItems(instrumentid, cycle);

    LOG(INFO) << "get_histories_count found " << histories.size();
    lua_pushnumber(L, histories.size());
    return 1;
}

// 得到某个品种某个周期上某天的某个指标值（指标值可以是多个，就需要多push几次）
static int get_histories_ma_value(lua_State *L)
{
	// pop得到参数,第一个参数是品种
	string instrumentid = lua_tostring(L, 1);
	if (instrumentid.empty())
	{
		LOG(ERROR) << "get_histories_ma_value instrumentid empty";
		lua_pushnumber(L, -1); //-1表示没有找到，外围程序自己处理当天的
		return 1;
	}
	// pop得到参数,第二个参数是周期，整数，分钟的倍数,日线就是24*60
	int cycle = lua_tointeger(L, 2);
	if (cycle == 0)
	{
		LOG(ERROR) << "get_histories_ma_value cytle zero";
		lua_pushnumber(L, -1); //-1表示没有找到，外围程序自己处理当天的
		return 1;
	}

	

	// pop得到参数,第3个参数是ma周期
	int macnt = lua_tointeger(L, 3);
	if (macnt < 0)
	{
		LOG(ERROR) << "get_histories_ma_value macnt has minus value. Valid value is 0(MA),1(MACD)";
		lua_pushnumber(L, -1); //-1表示没有找到，外围程序自己处理当天的
		return 1;
	}

	// pop得到参数,第4个是历史数据索引
	int history_index = lua_tointeger(L, 4);
	if (history_index < 0)
	{
		LOG(ERROR) << "get_histories_ma_value history_index minus value.";
		lua_pushnumber(L, -1); //-1表示没有找到，外围程序自己处理当天的
		return 1;
	}

	HistoryMysqlDb &hsd = strategy_context::get_mutable_instance().get_history_mysql_db();

	// 第二个参数是分钟，一天就是24*60分钟
	std::vector<HistoryItem*> &histories = hsd.getHistoryItems(instrumentid, cycle);

	
	
	hsd.CalculateMAIndexType(instrumentid.c_str(), cycle, macnt);

	if (history_index >= histories.size())
		history_index = histories.size() - 1;

	if (histories[history_index]->ma_values.find(macnt) != histories[history_index]->ma_values.end())
	{
		LOG(INFO) << "get_histories_ma_value found " << macnt<<":"<< histories[history_index]->ma_values[macnt];
		lua_pushnumber(L, histories[history_index]->ma_values[macnt]);
	}
	else //找不到的话，使用上一个的
	{
		if (histories[history_index - 1]->ma_values.find(macnt) != histories[history_index - 1]->ma_values.end())
		{
			LOG(INFO) << "get_histories_ma_value use previous one " << macnt << ":" << histories[history_index-1]->ma_values[macnt];
			lua_pushnumber(L, histories[history_index-1]->ma_values[macnt]);
		}else
			lua_pushnumber(L, -1);
	}
	

	
	return 1;
}


// 第一版 所有给lua脚本的接口都实现在这里吧
static int get_script_value(lua_State *L)
{
	// pop得到参数
	string investor = lua_tostring(L, 1);
	if (investor.empty())
	{
		LOG(ERROR) << "get_script_value investor id empty";
		return 0;
	}

	string param_name = lua_tostring(L, 2);
	if (param_name.empty())
	{
		LOG(ERROR) << "get_script_value parameter name empty";
		return 0;
	}

	// 本地文件名
	std::string filename = K_PARAMETER_FOLDER + investor + "_";
	filename += param_name + my_utils::K_STRATEGY_PARAMETER_FILE_EXTENSION;

	// 打开文件保存
	ifstream l_file(filename);
	if (!l_file)
	{
		LOG(ERROR) << filename + " open failed";
		return 0;
	}

	std::string str((std::istreambuf_iterator<char>(l_file)),
		std::istreambuf_iterator<char>());
	l_file.close();
	
	lua_pushstring(L, str.c_str());
	return 1;
}

// 保存脚本参数
static int save_script_value(lua_State *L)
{
	// pop得到参数
	string investor = lua_tostring(L, 1);
	if (investor.empty())
	{
		LOG(ERROR) << "save_script_value investor id empty";
		return 0;
	}

	string param_name = lua_tostring(L, 2);
	if (param_name.empty())
	{
		LOG(ERROR) << "save_script_value parameter name empty";
		return 0;
	}

	string param_value = lua_tostring(L, 3);
	if (param_value.empty())
	{
		LOG(ERROR) << "save_script_value parameter value empty";
		return 0;
	}
	
	// 考虑怎么存，先写本地文件吧
	std::string filename = K_PARAMETER_FOLDER + investor+"_";
	filename += param_name + my_utils::K_STRATEGY_PARAMETER_FILE_EXTENSION;

	// 打开文件保存
	ofstream l_file(filename, ios::in | ios::out | ios::trunc);
	if (!l_file)
	{
		LOG(ERROR)<<filename + " save failed";
		return 0;
	}
	l_file << param_value;
	l_file.close();

	return 0;
}

static long get_file_size(const char* filename)
{
	std::ifstream in(filename);
	if (!in.is_open()) return -1;

	in.seekg(0, std::ios_base::end);
	std::streampos pos= in.tellg();
	in.close();
	return pos;
}
lua_helper::lua_helper()
{
}


lua_helper::~lua_helper()
{
}


void lua_helper::invoke_script(const char *filename, bool finished, float open, float high, float low, float close, const char *trade_date, int &buyCnt, int&sellCnt)
{
	if (get_file_size(filename) <= 0) return;

	/*1. 加载文件并运行*/
	//int ret = luaL_loadfile(L, filename);
	std::string content = my_utils::get_file_content(filename);
	LOG(DEBUG) <<"Executing lua strategy file:" <<filename;
	invoke_script(content, finished, open, high, low, close, trade_date, buyCnt, sellCnt);
	LOG(DEBUG) << filename << ": buy counter = " << buyCnt << ";sell counter =" << sellCnt;

}

void lua_helper::invoke_script(	const std::string &content,
	bool finished,
	float open,
	float high,
	float low,
	float close,
	const char *trade_date,
	int &buyCnt,
	int&sellCnt)
{
	//1. 打开Lua  
	lua_State *L = luaL_newstate();
	/*2. 加载lua所有库*/
	luaL_openlibs(L);
	// 注册库函数
	lua_register(L, "get_data_of_days_before", get_data_of_days_before);
	lua_register(L, "get_data_of_index", get_data_of_index);
	lua_register(L, "get_script_value", get_script_value);
	lua_register(L, "save_script_value", save_script_value);
	lua_register(L, "get_trade_date_distance", get_trade_date_distance);
	lua_register(L, "get_histories_count", get_histories_count);
	lua_register(L, "get_histories_ma_value", get_histories_ma_value);
    lua_register(L, "get_history_data_datetime", get_history_data_datetime);
    lua_register(L, "get_trade_time_distance", get_trade_time_distance);

	int ret = luaL_loadstring(L, content.c_str());
	if (ret)
	{
		LOG(ERROR) << "Load lua script file failed";
		return;
	}

	//lua_pushboolean(L, finished);
	//lua_pushnumber(L, open);          // 压入第1个参数  
	//lua_pushnumber(L, high);          // 压入第2个参数  
	//lua_pushnumber(L, low);          // 压入第3个参数  
	//lua_pushnumber(L, close);          // 压入第4个参数 
	//lua_pushstring(L, trade_date);
	ret = lua_pcall(L, 0, 0, 0);
	if (ret)
	{
		LOG(ERROR) << "Lua pcall error for file: ";
		return;
	}

	////4. 执行函数
	lua_getglobal(L, "main");        // 获取函数，压入栈中 
	lua_pushboolean(L, finished);
	lua_pushnumber(L, open);          // 压入第1个参数  
	lua_pushnumber(L, high);          // 压入第2个参数  
	lua_pushnumber(L, low);          // 压入第3个参数  
	lua_pushnumber(L, close);          // 压入第4个参数 
	lua_pushstring(L, trade_date);
	ret = lua_pcall(L, 6, 2, 0);// 调用函数，调用完成以后，会将返回值压入栈中，2表示参数个数，1表示返回结果个数。  

	if (ret)                       // 调用出错  
	{
		const char *pErrorMsg = lua_tostring(L, -1);
		LOG(ERROR) << "lua_pcall main Error:" << pErrorMsg;
		lua_close(L);
		return;
	}

	// int lua script, return buyCnt,sellCnt
	// if you only return num, num will be buyCnt
	buyCnt = lua_tointeger(L, -2);
	sellCnt = lua_tointeger(L, -1);
	// 得到这个后，写入数据库(or redis)
	//lua_setfield(L,)
	/*清除lua*/
	lua_close(L);
}