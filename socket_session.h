#ifndef  SOCKET_SESSION_H__
#define SOCKET_SESSION_H__
#include <string>
#include <boost\shared_ptr.hpp>
#include <boost\noncopyable.hpp>
#include <boost\enable_shared_from_this.hpp>
#include <boost\function.hpp>
#include <boost/asio.hpp>
#include <boost\array.hpp>

//http://www.cnblogs.com/lidabo/p/3791159.html
enum command { heartbeat = 0, regist, normal };

class message;
class socket_session;
typedef boost::shared_ptr<socket_session> socket_session_ptr;

class  socket_session :
	public boost::enable_shared_from_this<socket_session>,
	private boost::noncopyable
{
public:
	typedef boost::function<void(socket_session_ptr)> close_callback;
	typedef boost::function<void(
		const boost::system::error_code&,
		socket_session_ptr, message&)> read_data_callback;

	socket_session(boost::asio::io_service& io_service);
	~socket_session(void);

	std::string& get_remote_addr() { return m_name; }
	void set_remote_addr(std::string& name) { m_name = name; }
	boost::asio::ip::tcp::socket& socket() { return m_socket; }

	void installCloseCallBack(close_callback cb) { close_cb = cb; }
	void installReadDataCallBack(read_data_callback cb) { read_data_cb = cb; }

	void start();
	void close();
	void async_write(const std::string& sMsg);
	void async_write(message& msg);

	bool is_timeout();
	void set_op_time() { std::time(&m_last_op_time); }
public:
	std::string m_name;
private:
	/*DWORD m_id;
	WORD  m_business_type;
	DWORD m_app_id;*/
	
	/*boost::array<char, 7> sHeader;
	std::string sBody;*/

	boost::asio::ip::tcp::socket m_socket;
	boost::asio::io_service& m_io_service;

	std::time_t m_last_op_time;

	close_callback close_cb;
	read_data_callback read_data_cb;

	//发送消息  
	void handle_write(const boost::system::error_code& e,
		std::size_t bytes_transferred, std::string* pmsg);

	//读消息头  
	void handle_read_header(const boost::system::error_code& error);
	//读消息体  
	void handle_read_body(const boost::system::error_code& error);

	void handle_close();
};
#endif // ! SOCKET_SESSION_H__