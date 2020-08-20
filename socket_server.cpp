#include "socket_server.h"
#include <iostream>
#include <string>
#include <memory>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#define ELPP_THREAD_SAFE  
#include "easylogging++.h"

socket_server::socket_server(boost::asio::io_service &iosvc, const int _port)
	:socket_io(iosvc), port(_port), tcp_acceptor(iosvc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), _port))
	, m_manager(iosvc,100)
{
	
}


socket_server::~socket_server()
{
}


void socket_server::start()
{
	//boost::shared_ptr<boost::asio::ip::tcp::socket> client(new boost::asio::ip::tcp::socket(socket_io));
	socket_session_ptr new_session(new socket_session(socket_io));
	tcp_acceptor.async_accept(new_session->socket(), boost::bind(&socket_server::handle_accept, this, new_session, boost::asio::placeholders::error));
}

void socket_server::handle_accept(socket_session_ptr session, const boost::system::error_code& error)
{
	try
	{
		start();
		if (session != nullptr)
		{
			session->installCloseCallBack(boost::bind(&socket_server::close_callback, this, _1));
			m_manager.add_session(session);
		}
	}
	catch (std::exception& e)
	{
		LOG(ERROR)<< "socket异常:[" << e.what() << "]";
	}
	catch (...)
	{
		LOG(ERROR) << "socket异常:[未知异常]";
	}

}


void socket_server::close_callback(socket_session_ptr session)
{
	LOG(INFO)<<"close_callback";
	try {
		m_manager.del_session(session);
	}
	catch (std::exception& e)
	{
		LOG(INFO) << "socket异常:[" << e.what() << "]";
	}
	catch (...)
	{
		LOG(INFO) << "socket异常:[未知异常]";
	}

}