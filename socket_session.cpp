#include "socket_session.h"
#include <boost/bind.hpp>
#define ELPP_THREAD_SAFE  
#include "easylogging++.h"

socket_session::socket_session(boost::asio::io_service& io_service):m_io_service(io_service), m_socket(io_service)
{
}


socket_session::~socket_session()
{
	m_socket.close();
}

void socket_session::start()
{
	m_socket.set_option(boost::asio::ip::tcp::acceptor::linger(true, 0));
	m_socket.set_option(boost::asio::socket_base::keep_alive(true));
	std::time(&m_last_op_time);
	const boost::system::error_code error;
	handle_read_header(error);
}

void socket_session::handle_close()
{
	try {
		m_socket.close();
		close_cb(shared_from_this());
	}
	catch (std::exception& e)
	{
		LOG(ERROR) << "����Զ�̵�ַ:[" << get_remote_addr() << "],socket�쳣:[" << e.what() << "]";
	}
	catch (...)
	{
		LOG(ERROR)<< "����Զ�̵�ַ:[" << get_remote_addr() << "],socket�쳣:[δ֪�쳣]";
	}
}

void socket_session::close()
{
	//���ڻص����м���������������ύ������һ���߳�ȥ������Ȼ���������  
	m_io_service.post(boost::bind(&socket_session::handle_close, shared_from_this()));
}

static int connection_timeout = 60;

bool socket_session::is_timeout()
{
	std::time_t now;
	std::time(&now);
	return now - m_last_op_time > connection_timeout;
}

//����Ϣͷ  
void socket_session::handle_read_header(const boost::system::error_code& error)
{
	LOG(DEBUG)<<__FUNCDNAME__<< " enter.";

	try {
		if (error)
		{
			LOG(ERROR) << "����Զ�̵�ַ:[" << get_remote_addr() << "],socket�쳣:[" << error.message().c_str() << "]";
			close();
			return;
		}

		//std::string data;
		//data.swap(sBody);
		//boost::asio::async_read(m_socket,
		//	boost::asio::buffer(sHeader),
		//	boost::bind(&socket_session::handle_read_body, shared_from_this(),
		//		boost::asio::placeholders::error));

		//if (data.length() > 0 && data != "")
		//{//�������ݻص�ע���READ_DATA����  
		//	message msg;
		//	message_iarchive(msg, data);

		//	read_data_cb(error, shared_from_this(), msg);
		//}
	}
	catch (std::exception& e)
	{
		LOG(ERROR) << "����Զ�̵�ַ:[" << get_remote_addr() << "],socket�쳣:[" << e.what() << "]";
		close();
	}
	catch (...)
	{
		LOG(ERROR) << "����Զ�̵�ַ:[" << get_remote_addr() << "],socket�쳣:[δ֪�쳣]";
		close();
	}
}

//����Ϣ��  
void socket_session::handle_read_body(const boost::system::error_code& error)
{}
//{
//	LOG(DEBUG) << __FUNCDNAME__ << " enter.";
//
//	try {
//		if (error)
//		{
//			LOG(ERROR)<< "����Զ�̵�ַ:[" << get_remote_addr() << "],socket�쳣:[" << error.message().c_str() << "]";
//			close();
//			return;
//		}
//
//		if (tag.compare(0, tag.length(), sHeader.data(), 0, tag.length()))
//		{
//			LOG(ERROR)<< "����Զ�̵�ַ:[" << get_remote_addr() << "],socket�쳣:[���Ǹ��Ƿ�����!]");
//			close();
//			return;
//		}
//
//		DWORD dwLength = 0;
//
//		char* len = (char*) &dwLength;
//		memcpy(len, &sHeader[tag.length()], sizeof(dwLength));
//
//		sBody.resize(dwLength);
//		char* pBody = &sBody[0];
//
//		boost::asio::async_read(m_socket,
//			boost::asio::buffer(pBody, dwLength),
//			boost::bind(&socket_session::handle_read_header, shared_from_this(),
//				boost::asio::placeholders::error));
//	}
//	catch (std::exception& e)
//	{
//		LOG(ERROR)<< "����Զ�̵�ַ:[" << get_remote_addr() << "],socket�쳣:[" << e.what() << "]";
//		close();
//	}
//	catch (...)
//	{
//		LOG(ERROR) << "����Զ�̵�ַ:[" << get_remote_addr() << "],socket�쳣:[δ֪�쳣]";
//		close();
//	}
//}

void socket_session::handle_write(const boost::system::error_code& error,
	std::size_t bytes_transferred, std::string* pmsg)
{
	//���ݷ��ͳɹ�������  
	if (pmsg != NULL)
	{
		delete pmsg;
	}

	if (error)
	{
		LOG(ERROR)<< "����Զ�̵�ַ:[" << get_remote_addr() << "],socket�쳣:[" << error.message().c_str() << "]";
		close();
		return;
	}
}

void socket_session::async_write(const std::string& sMsg)
{
	LOG(DEBUG) << __FUNCDNAME__ << " enter.";
	try
	{
		DWORD dwLength = sMsg.size();
		char* pLen = (char*) &dwLength;

		//�������첽���ͣ�Ҫ��֤���ݷ�������ʱ���Ű���������  
		std::string* msg = new std::string();
		msg->append(pLen, sizeof(dwLength));
		msg->append(sMsg);

		boost::asio::async_write(m_socket, boost::asio::buffer(*msg, msg->size()),
			boost::bind(&socket_session::handle_write, shared_from_this(),
				boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred,
				msg));

	}
	catch (std::exception& e)
	{
		LOG(ERROR)<<"����Զ�̵�ַ:[" << get_remote_addr() << "],socket�쳣:[" << e.what() << "]";
		close();
	}
	catch (...)
	{
		LOG(ERROR) << "����Զ�̵�ַ:[" << get_remote_addr() << "],socket�쳣:[δ֪�쳣]";
		close();
	}
}

void socket_session::async_write(message& msg)
{
	std::string data;
	//message_oarchive(data, msg);

	async_write(data);
}