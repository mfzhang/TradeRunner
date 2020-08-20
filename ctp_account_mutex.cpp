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
		myfile.close(); //�Ȳ�ɾ�ˣ�������exclusive ��
	}
}
