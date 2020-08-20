#include "session_manager.h"

session_manager::session_manager(boost::asio::io_service& io_srv, int expires_time)
	:m_io_srv(io_srv), 
	m_check_tick(io_srv), 
	m_expires_time(expires_time), 
	m_next_session(0)
{
}


session_manager::~session_manager()
{
}
