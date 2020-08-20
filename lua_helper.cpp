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
// ��һ�� ���и�lua�ű��Ľӿڶ�ʵ���������
static int get_data_of_days_before(lua_State *L)
{
	// pop�õ�����
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

	// �ڶ��������Ƿ��ӣ�һ�����24*60����
	std::vector<HistoryItem*> &histories = hsd.getHistoryItems(item, 24 * 60);

	// �ս��У��
	if (histories.empty())
	{
		LOG(ERROR) << item << " history day data empty";
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		return 4;
	}

	// ����У��
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

// ��һ�� ���и�lua�ű��Ľӿڶ�ʵ���������
static int get_data_of_index(lua_State *L)
{
	// pop�õ�����
	string item = lua_tostring(L, 1);
	if (item.empty())
	{
		LOG(ERROR) << "get_data_of_index instrument id empty";
		return 0;
	}
	// pop�õ�����,�ڶ������������ڣ����������ӵı���,���߾���24*60
	int cycle = lua_tointeger(L, 2);
	if (cycle <= 0)
	{
		LOG(ERROR) << "get_data_of_index cytle zero";
		lua_pushnumber(L, -1); //-1��ʾû���ҵ�����Χ�����Լ��������
		return 1;
	}

	int index = lua_tointeger(L, 3);
	if (index < 0)
	{
		LOG(ERROR) << "get_data_of_index index less than zero";
		lua_pushnumber(L, -1); //-1��ʾû���ҵ�����Χ�����Լ��������
		return 1;
	}

	/*lua_pushinteger(L, 100 + i);
	lua_pushinteger(L, 200 + i);
	lua_pushinteger(L, 300 + i);
	lua_pushinteger(L, 400 + i);
	*/

	HistoryMysqlDb &hsd = strategy_context::get_mutable_instance().get_history_mysql_db();

	// �ڶ��������Ƿ��ӣ�һ�����24*60����
	std::vector<HistoryItem*> &histories = hsd.getHistoryItems(item, cycle);

	// �ս��У��
	if (histories.empty())
	{
		LOG(ERROR) << item << " history day data empty";
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		return 4;
	}

	// ����У��
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
�ú������ݴ��������(YYYYMMDD)�ͺ�Լ���࣬�õ�����������ʷ�о�����������
����˵��������������ڷ���1
*/
static int get_trade_date_distance(lua_State *L)
{
	// pop�õ�����
	string trade_date = lua_tostring(L, 1);
	if (trade_date.empty())
	{
		LOG(ERROR) << "get_trade_date_distance trade date empty";
		lua_pushnumber(L, -1); //-1��ʾû���ҵ�����Χ�����Լ��������
		return 1;
	}

	string instrumentid = lua_tostring(L, 2);
	if (instrumentid.empty())
	{
		LOG(ERROR) << "get_trade_date_distance instrumentid empty";
		lua_pushnumber(L, -1); //-1��ʾû���ҵ�����Χ�����Լ��������
		return 1;
	}

	HistoryMysqlDb &hsd = strategy_context::get_mutable_instance().get_history_mysql_db();

	// �ڶ��������Ƿ��ӣ�һ�����24*60����
	std::vector<HistoryItem*> &histories = hsd.getHistoryItems(instrumentid, 24 * 60);

	// �ս��У�飬��Ϊ�����
	if (histories.empty())
	{
		LOG(ERROR) << "get_trade_date_distance item history empty";
		lua_pushnumber(L, -1); //-1��ʾû���ҵ�����Χ�����Լ��������
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
	lua_pushnumber(L,  -1);//-1��ʾû���ҵ�����Χ�����Լ��������
	return 1;
}


/*
�ú������ݴ����ʱ��(YYYYMMDD hhmmss),��k�����ںͺ�Լ���࣬�õ�����������ʷ�о�����������
����˵��������������ڷ���1
*/
static int get_trade_time_distance(lua_State *L)
{
    

    string instrumentid = lua_tostring(L, 1);
    if (instrumentid.empty())
    {
        LOG(ERROR) << "get_trade_time_distance instrumentid empty";
        lua_pushnumber(L, -1); //-1��ʾû���ҵ�����Χ�����Լ��������
        return 1;
    }

    // pop�õ�����,�ڶ������������ڣ����������ӵı���,���߾���24*60
    int cycle = lua_tointeger(L, 2);
    if (cycle == 0)
    {
        LOG(ERROR) << "get_trade_time_distance cycle zero";
        lua_pushnumber(L, -1); //-1��ʾû���ҵ�����Χ�����Լ��������
        return 1;
    }

    // pop�õ�����
    string trade_date = lua_tostring(L, 3);
    if (trade_date.empty())
    {
        LOG(ERROR) << "get_trade_time_distance trade time empty";
        lua_pushnumber(L, -1); //-1��ʾû���ҵ�����Χ�����Լ��������
        return 1;
    }

    HistoryMysqlDb &hsd = strategy_context::get_mutable_instance().get_history_mysql_db();

    // �ڶ��������Ƿ��ӣ�һ�����24*60����
    std::vector<HistoryItem*> &histories = hsd.getHistoryItems(instrumentid, cycle);

    // �ս��У�飬��Ϊ�����
    if (histories.empty())
    {
        LOG(ERROR) << "get_trade_time_distance item history empty";
        lua_pushnumber(L, -1); //-1��ʾû���ҵ�����Χ�����Լ��������
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
            //�ҵ��˷���
            lua_pushnumber(L, histories.size() - i);
            return 1;
        }
    }

    LOG(ERROR) << "get_trade_time_distance not found ";
    lua_pushnumber(L, -1);//-1��ʾû���ҵ�����Χ�����Լ��������
    return 1;
}

//����Ʒ�ֺ������õ�ʱ���ַ���
static int get_history_data_datetime(lua_State *L) 
{
    // pop�õ�����,��һ��������Ʒ��
    string instrumentid = lua_tostring(L, 1);
    if (instrumentid.empty())
    {
        LOG(ERROR) << "get_history_data_datetime instrumentid empty";
        lua_pushnumber(L, -1); //-1��ʾû���ҵ�����Χ�����Լ��������
        return 1;
    }
    // pop�õ�����,�ڶ������������ڣ����������ӵı���,���߾���24*60
    int cycle = lua_tointeger(L, 2);
    if (cycle == 0)
    {
        LOG(ERROR) << "get_history_data_datetime cytle zero";
        lua_pushnumber(L, -1); //-1��ʾû���ҵ�����Χ�����Լ��������
        return 1;
    }


    // pop�õ�����,��4������ʷ��������
    int history_index = lua_tointeger(L, 3);
    if (history_index < 0)
    {
        LOG(ERROR) << "get_history_data_datetime history_index minus value.";
        lua_pushnumber(L, -1); //-1��ʾû���ҵ�����Χ�����Լ��������
        return 1;
    }

    HistoryMysqlDb &hsd = strategy_context::get_mutable_instance().get_history_mysql_db();

    // �ڶ��������Ƿ��ӣ�һ�����24*60����
    std::vector<HistoryItem*> &histories = hsd.getHistoryItems(instrumentid, cycle);

    if (history_index >= histories.size())
    {
        LOG(ERROR) << "get_history_data_datetime history_index exceed.";
        lua_pushstring(L, ""); //-1��ʾû���ҵ�����Χ�����Լ��������
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
// �õ�ĳ��Ʒ��ĳ�������ϵ���ʷ���ݵĳ���
static int get_histories_count(lua_State *L)
{
    // pop�õ�����,��һ��������Ʒ��
    string instrumentid = lua_tostring(L, 1);
    if (instrumentid.empty())
    {
        LOG(ERROR) << "get_histories_count instrumentid empty";
        lua_pushnumber(L, -1); //-1��ʾû���ҵ�����Χ�����Լ��������
        return 1;
    }
    // pop�õ�����,�ڶ������������ڣ����������ӵı���,���߾���24*60
    int cycle = lua_tointeger(L, 2);
    if (cycle == 0)
    {
        LOG(ERROR) << "get_histories_count cycle zero";
        lua_pushnumber(L, -1); //-1��ʾû���ҵ�����Χ�����Լ��������
        return 1;
    }

    HistoryMysqlDb &hsd = strategy_context::get_mutable_instance().get_history_mysql_db();

    // �ڶ��������Ƿ��ӣ�һ�����24*60����
    std::vector<HistoryItem*> &histories = hsd.getHistoryItems(instrumentid, cycle);

    LOG(INFO) << "get_histories_count found " << histories.size();
    lua_pushnumber(L, histories.size());
    return 1;
}

// �õ�ĳ��Ʒ��ĳ��������ĳ���ĳ��ָ��ֵ��ָ��ֵ�����Ƕ��������Ҫ��push���Σ�
static int get_histories_ma_value(lua_State *L)
{
	// pop�õ�����,��һ��������Ʒ��
	string instrumentid = lua_tostring(L, 1);
	if (instrumentid.empty())
	{
		LOG(ERROR) << "get_histories_ma_value instrumentid empty";
		lua_pushnumber(L, -1); //-1��ʾû���ҵ�����Χ�����Լ��������
		return 1;
	}
	// pop�õ�����,�ڶ������������ڣ����������ӵı���,���߾���24*60
	int cycle = lua_tointeger(L, 2);
	if (cycle == 0)
	{
		LOG(ERROR) << "get_histories_ma_value cytle zero";
		lua_pushnumber(L, -1); //-1��ʾû���ҵ�����Χ�����Լ��������
		return 1;
	}

	

	// pop�õ�����,��3��������ma����
	int macnt = lua_tointeger(L, 3);
	if (macnt < 0)
	{
		LOG(ERROR) << "get_histories_ma_value macnt has minus value. Valid value is 0(MA),1(MACD)";
		lua_pushnumber(L, -1); //-1��ʾû���ҵ�����Χ�����Լ��������
		return 1;
	}

	// pop�õ�����,��4������ʷ��������
	int history_index = lua_tointeger(L, 4);
	if (history_index < 0)
	{
		LOG(ERROR) << "get_histories_ma_value history_index minus value.";
		lua_pushnumber(L, -1); //-1��ʾû���ҵ�����Χ�����Լ��������
		return 1;
	}

	HistoryMysqlDb &hsd = strategy_context::get_mutable_instance().get_history_mysql_db();

	// �ڶ��������Ƿ��ӣ�һ�����24*60����
	std::vector<HistoryItem*> &histories = hsd.getHistoryItems(instrumentid, cycle);

	
	
	hsd.CalculateMAIndexType(instrumentid.c_str(), cycle, macnt);

	if (history_index >= histories.size())
		history_index = histories.size() - 1;

	if (histories[history_index]->ma_values.find(macnt) != histories[history_index]->ma_values.end())
	{
		LOG(INFO) << "get_histories_ma_value found " << macnt<<":"<< histories[history_index]->ma_values[macnt];
		lua_pushnumber(L, histories[history_index]->ma_values[macnt]);
	}
	else //�Ҳ����Ļ���ʹ����һ����
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


// ��һ�� ���и�lua�ű��Ľӿڶ�ʵ���������
static int get_script_value(lua_State *L)
{
	// pop�õ�����
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

	// �����ļ���
	std::string filename = K_PARAMETER_FOLDER + investor + "_";
	filename += param_name + my_utils::K_STRATEGY_PARAMETER_FILE_EXTENSION;

	// ���ļ�����
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

// ����ű�����
static int save_script_value(lua_State *L)
{
	// pop�õ�����
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
	
	// ������ô�棬��д�����ļ���
	std::string filename = K_PARAMETER_FOLDER + investor+"_";
	filename += param_name + my_utils::K_STRATEGY_PARAMETER_FILE_EXTENSION;

	// ���ļ�����
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

	/*1. �����ļ�������*/
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
	//1. ��Lua  
	lua_State *L = luaL_newstate();
	/*2. ����lua���п�*/
	luaL_openlibs(L);
	// ע��⺯��
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
	//lua_pushnumber(L, open);          // ѹ���1������  
	//lua_pushnumber(L, high);          // ѹ���2������  
	//lua_pushnumber(L, low);          // ѹ���3������  
	//lua_pushnumber(L, close);          // ѹ���4������ 
	//lua_pushstring(L, trade_date);
	ret = lua_pcall(L, 0, 0, 0);
	if (ret)
	{
		LOG(ERROR) << "Lua pcall error for file: ";
		return;
	}

	////4. ִ�к���
	lua_getglobal(L, "main");        // ��ȡ������ѹ��ջ�� 
	lua_pushboolean(L, finished);
	lua_pushnumber(L, open);          // ѹ���1������  
	lua_pushnumber(L, high);          // ѹ���2������  
	lua_pushnumber(L, low);          // ѹ���3������  
	lua_pushnumber(L, close);          // ѹ���4������ 
	lua_pushstring(L, trade_date);
	ret = lua_pcall(L, 6, 2, 0);// ���ú�������������Ժ󣬻Ὣ����ֵѹ��ջ�У�2��ʾ����������1��ʾ���ؽ��������  

	if (ret)                       // ���ó���  
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
	// �õ������д�����ݿ�(or redis)
	//lua_setfield(L,)
	/*���lua*/
	lua_close(L);
}