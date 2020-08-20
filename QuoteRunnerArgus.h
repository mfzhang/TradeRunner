#ifndef QUOTERUNNERARGUS_H
#define QUOTERUNNERARGUS_H
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

class QuoteRunnerArgus
{
private:
	//po::options_description desc;
	po::variables_map vm;
	std::vector<std::string> ctp_addresses;

public:
	QuoteRunnerArgus(){}
	void parse(int argc, char* argv[])
	{
		po::options_description desc("Allowed options");
		desc.add_options()
			("help", "produce help message")
			(my_utils::K_USERNAME, po::value<std::string>(), "username")
			(my_utils::K_PASSWORD, po::value<std::string>(), "password")
			(my_utils::K_ADDRESS, po::value<std::string>(), "ctp address")
			(my_utils::K_BROKER, po::value<std::string>(), "broker id")
			(my_utils::K_REDIS_ADDRESS, po::value<std::string>(), "redis server address")
			(my_utils::K_REDISPORT, po::value<int>(), "redis server port")
			(my_utils::K_DB_STRING, po::value<std::string>(), "mysql address")
			(my_utils::K_DB_USERNAME, po::value<std::string>(), "mysql username")
			(my_utils::K_DB_PWD, po::value<std::string>(), "mysql password");

		try
		{
			po::store(po::parse_command_line(argc, argv, desc), vm);
			po::notify(vm);

			if (vm.count(my_utils::K_ADDRESS))
			{
				boost::split(ctp_addresses, vm[my_utils::K_ADDRESS].as<std::string>(), boost::is_any_of(" ,;"), boost::token_compress_on);
			}
		}catch(...)
		{ }

		if (!isvalid())
		{
			std::cout << desc << std::endl;
		}
	}
	
	const std::string get_username()
	{
		if (vm.count(my_utils::K_USERNAME))
		{
			return vm[my_utils::K_USERNAME].as<std::string>();
		}
		return "";
	}

	const std::string get_password()
	{
		if (vm.count(my_utils::K_PASSWORD))
		{
			return vm[my_utils::K_PASSWORD].as<std::string>();
		}

		return "";
	}

	const std::string get_broker()
	{
		if (vm.count(my_utils::K_BROKER))
		{
			return vm[my_utils::K_BROKER].as<std::string>();
		}
		return "";
	}

	
	const std::vector<std::string>&get_addresses()
	{
		return ctp_addresses;
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
		if (get_db_connection_string().length() <= 0
			|| get_db_user().length() <= 0
			|| get_db_pwd().length() <= 0) return false;
			
		return !get_username().empty() && !get_password().empty() && !get_addresses().empty();
	}
};
#endif