#ifndef TRADE_STRATEGY_ARGUS_H__
#define TRADE_STRATEGY_ARGUS_H__
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/format.hpp>  
#include <boost/tokenizer.hpp>  
#include <boost/algorithm/string.hpp>  
#include <string>
#include <iostream>
#include "my_utils.h"
namespace po = boost::program_options;

class trade_strategy_argus
{
private:
	//po::options_description desc;
	po::variables_map vm;
	std::vector<std::string> ctp_addresses;
	static std::string local_ip_address;

public:
	
	trade_strategy_argus() {}
	void parse(int argc, char* argv [])
	{
		po::options_description desc("Allowed options");
		desc.add_options()
			("help", "produce help message")
			(my_utils::K_REDIS_ADDRESS, po::value<std::string>(), "redis server address")
			(my_utils::K_REDISPORT, po::value<int>(), "redis server port")
			(my_utils::K_MODE_ONLY_REFRESH_STRATETIES,"refresh strategy")
			(my_utils::K_USERNAME, po::value<std::string>(), "CTP account")
			(my_utils::K_DB_STRING, po::value<std::string>(), "mysql address")
			(my_utils::K_DB_USERNAME, po::value<std::string>(), "mysql username")
			(my_utils::K_DB_PWD, po::value<std::string>(), "mysql password")
			("list", po::value<int>(), "strategy lists");

		try
		{
			po::store(po::parse_command_line(argc, argv, desc), vm);
			po::notify(vm);
		}
		catch (...)
		{
		}

		if (!isvalid())
		{
			std::cout << desc << std::endl;
		}
	}

	

	const std::string get_redis_address()
	{
		if (vm.count(my_utils::K_REDIS_ADDRESS))
		{
			return vm[my_utils::K_REDIS_ADDRESS].as<std::string>();
		}

		return "127.0.0.1";
	}

	const int get_redis_port()
	{
		if (vm.count(my_utils::K_REDISPORT))
		{
			return vm[my_utils::K_REDISPORT].as<int>();
		}

		return 6379;
	}

	const std::string get_current_investor()
	{
		if (vm.count(my_utils::K_USERNAME))
		{
			return vm[my_utils::K_USERNAME].as<std::string>();
		}

		return "";
	}


	const bool is_only_refresh_strategy()
	{
		if (vm.count(my_utils::K_MODE_ONLY_REFRESH_STRATETIES))
			return true;

		return false;
	}

	const std::string get_db_connection_string()
	{
		if (vm.count(my_utils::K_DB_STRING))
		{
			return vm[my_utils::K_DB_STRING].as<std::string>();
		}

		return "";
	}
	/*const std::string get_db_connection_string()
	{
		if (vm.count(my_utils::K_DB_STRING))
			return vm[my_utils::K_DB_STRING].as<std::string>();

		return "";
	}*/

	const std::string get_db_user()
	{
		if (vm.count(my_utils::K_DB_USERNAME))
			return vm[my_utils::K_DB_USERNAME].as<std::string>();

		return "";
	}
	const std::string get_db_pwd()
	{
		if (vm.count(my_utils::K_DB_PWD))
			return vm[my_utils::K_DB_PWD].as<std::string>();

		return "";
	}

	bool isvalid()
	{
		if(is_only_refresh_strategy()) 
			return true;
		if (get_current_investor().length() <= 0)
			return false;

		if (get_db_connection_string().length() <= 0
			|| get_db_user().length() <= 0
			|| get_db_pwd().length() <= 0) return false;

		return true;
	}


};
#endif