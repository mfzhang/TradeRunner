#include "ctp_account_mutex.h"
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif


ctp_account_mutex::ctp_account_mutex()
{
	_pid = _getpid();
}


ctp_account_mutex::~ctp_account_mutex()
{
	if (myfile.good())
	{
		myfile.close(); //先不删了，反正是exclusive 打开
	}
}
