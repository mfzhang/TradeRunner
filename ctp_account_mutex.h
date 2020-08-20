#ifndef CTP_ACCOUNT_MUTEX_H__
#define CTP_ACCOUNT_MUTEX_H__
#include <boost/asio.hpp>
#include <fstream>
#include <exception>
#include <iostream>
#include <corecrt_share.h>

class ctp_account_mutex
{
private:
	/* boost asio*/
	//boost::asio::io_service &io_service;
	std::ofstream myfile;
	int _pid;
public:
	ctp_account_mutex();
	~ctp_account_mutex();

	// ***
	// return false means it is alreay running
	//		  true means it is run this time
	bool run()
	{
		try
		{
			myfile.open(_path + _file, std::ios::out | std::ios::trunc, _SH_DENYRW);
			if (!myfile.good())
			{
				//std::cerr << "File open failed, another instance is running!" << std::endl;
				return false;
			}
			myfile << _pid << std::endl;
		}
		catch (std::exception *e)
		{
			std::cerr << e->what();
			return false;
		}
		return true;
	}
	


public:
	std::string _path;
	std::string _file;
};

#endif // !CTP_ACCOUNT_MUTEX_H__