#ifndef  SOCKET_SERVER_H___
#define SOCKET_SERVER_H___
#include "session_manager.h"

#include <boost\asio.hpp>
#include "boost/function.hpp"

class socket_server
{
public:
	socket_server(boost::asio::io_service &iosvc, const int _port );
	~socket_server();

	void start();
private:
	void handle_accept(socket_session_ptr session, const boost::system::error_code& error);
	boost::asio::io_service &socket_io;
	int port;
	boost::asio::ip::tcp::acceptor tcp_acceptor;

	session_manager m_manager;

	void close_callback(socket_session_ptr session);  

};

#endif // ! SOCKET_SERVER_H___