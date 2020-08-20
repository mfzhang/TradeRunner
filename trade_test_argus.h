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

class trade_test_argus
{
private:
	//po::options_description desc;
	po::variables_map vm;
	std::vector<std::string> ctp_addresses;
	static std::string local_ip_address;

public:
	
	trade_test_argus() {}
	void parse(int argc, char* argv [])
	{
		po::options_description desc("Allowed options");
		desc.add_options()
			("help", "produce help message")
			(my_utils::K_USERNAME, po::value<std::string>(), "username")
			(my_utils::K_REDIS_ADDRESS, po::value<std::string>(), "redis server address")
			(my_utils::K_REDISPORT, po::value<int>(), "redis server port")
			("verify_lua", po::value<std::string>(), "验证lua编译可行性,lua文件绝对路径")
			("quote_simu", "模拟quote")
			("strategy_simu", po::value<std::string>(), "模拟strategy");

		try
		{
			po::store(po::parse_command_line(argc, argv, desc), vm);
			po::notify(vm);
		}
		catch (...)
		{
		}

		if (vm.count("help"))
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

	const std::string get_lua_filename()
	{
		if (vm.count("verify_lua"))
		{
			return vm["verify_lua"].as<std::string>();
		}

		return "";
	}

	const std::string get_strategy_simu()
	{
		if (vm.count("strategy_simu"))
		{
			return vm["strategy_simu"].as<std::string>();
		}

		return "";
	}

	const bool is_quote_simu()
	{
		return vm.count("quote_simu");
	}

	const std::string get_username()
	{
		if (vm.count(my_utils::K_USERNAME))
		{
			return vm[my_utils::K_USERNAME].as<std::string>();
		}
		return "";
	}

	bool isvalid()
	{
		
		return true;
	}


};
#endif