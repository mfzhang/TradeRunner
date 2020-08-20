#ifndef SESSION_MANAGER_H__
#define SESSION_MANAGER_H__
#include "socket_session.h"  
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/random.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/shared_lock_guard.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/signals2/detail/unique_lock.hpp>
class session_manager
{
public:
	typedef boost::shared_lock_guard<boost::shared_mutex> readLock;
	typedef boost::signals2::detail::unique_lock<boost::shared_mutex> writeLock;

	session_manager(boost::asio::io_service& io_srv, int expires_time);
	~session_manager();

	void add_session(socket_session_ptr p)
	{
		writeLock lock(m_mutex);
		socket_session_ptr stuSession;
		std::list<socket_session_ptr>::iterator it;
		for (it = m_sessions.begin(); it != m_sessions.end(); ++it)
		{
			if (it->get() == p.get())
			{
				return;
			}
		}
		m_sessions.push_back(stuSession);

	}
	/*void update_session(socket_session_ptr p)
	{
	}*/

	void del_session(socket_session_ptr m)
	{
		writeLock lock(m_mutex);
		if (m_sessions.empty())
		{
			return;
		}

		std::list<socket_session_ptr>::iterator it;
		for (it = m_sessions.begin(); it != m_sessions.end(); ++it)
		{
			if (it->get() == m.get())
			{
				m_sessions.erase(it);
				return;
			}
		}
	}

	////获取容器中的第一个session  
	//template<typename Tag, typename Member>
	//socket_session_ptr get_session(Member m)
	//{
	//	readLock lock(m_mutex);

	//	if (m_sessions.empty())
	//	{
	//		return socket_session_ptr();
	//	}

	//	MULTI_MEMBER_CON(Tag) idx = boost::multi_index::get<Tag>(m_sessions);
	//	BOOST_AUTO(iter, idx.find(m));
	//	return iter != boost::end(idx) ? iter->session : socket_session_ptr();
	//}

	////随机获取容器中的session  
	//template<typename Tag>
	//socket_session_ptr get_session_by_business_type(WORD m)
	//{
	//	typedef filter_container<is_business_type, MULTI_MEMBER_ITR(Tag)> FilterContainer;
	//	readLock lock(m_mutex);

	//	if (m_sessions.empty())
	//	{
	//		return socket_session_ptr();
	//	}

	//	MULTI_MEMBER_CON(Tag) idx = boost::multi_index::get<Tag>(m_sessions);

	//	//对容器的元素条件过滤  
	//	is_business_type predicate(m);
	//	FilterContainer fc(predicate, idx.begin(), idx.end());
	//	FilterContainer::FilterIter iter = fc.begin();

	//	if (fc.begin() == fc.end())
	//	{
	//		return socket_session_ptr();
	//	}

	//	int step = m_next_session % fc.szie();
	//	++m_next_session;

	//	for (int i = 0; i < step; ++i)
	//	{
	//		iter++;
	//	}

	//	return iter != fc.end() ? iter->session : socket_session_ptr();
	//}

	////根据类型和地址取session  
	//template<typename Tag>
	//socket_session_ptr get_session_by_type_ip(WORD m, std::string& ip)
	//{
	//	typedef filter_container<is_business_type, MULTI_MEMBER_ITR(Tag)> FilterContainer;
	//	readLock lock(m_mutex);

	//	if (m_sessions.empty())
	//	{
	//		return socket_session_ptr();
	//	}

	//	MULTI_MEMBER_CON(Tag) idx = boost::multi_index::get<Tag>(m_sessions);

	//	//对容器的元素条件过滤  
	//	is_business_type predicate(m);
	//	FilterContainer fc(predicate, idx.begin(), idx.end());
	//	FilterContainer::FilterIter iter = fc.begin();

	//	if (fc.begin() == fc.end())
	//	{
	//		return socket_session_ptr();
	//	}

	//	while (iter != fc.end())
	//	{
	//		if (iter->session->get_remote_addr().find(ip) != std::string::npos)
	//		{
	//			break;
	//		}

	//		iter++;
	//	}

	//	return iter != fc.end() ? iter->session : socket_session_ptr();
	//}

	////根据类型和app_id取session  
	//template<typename Tag>
	//socket_session_ptr get_session_by_type_appid(WORD m, DWORD app_id)
	//{
	//	typedef filter_container<is_business_type, MULTI_MEMBER_ITR(Tag)> FilterContainer;
	//	readLock lock(m_mutex);

	//	if (m_sessions.empty())
	//	{
	//		return socket_session_ptr();
	//	}

	//	MULTI_MEMBER_CON(Tag) idx = boost::multi_index::get<Tag>(m_sessions);

	//	//对容器的元素条件过滤  
	//	is_business_type predicate(m);
	//	FilterContainer fc(predicate, idx.begin(), idx.end());
	//	FilterContainer::FilterIter iter = fc.begin();

	//	if (fc.begin() == fc.end())
	//	{
	//		return socket_session_ptr();
	//	}

	//	while (iter != fc.end())
	//	{
	//		if (iter->session->get_app_id() == app_id)
	//		{
	//			break;
	//		}

	//		iter++;
	//	}

	//	return iter != fc.end() ? iter->session : socket_session_ptr();
	//}

private:
	int m_expires_time;
	boost::asio::io_service& m_io_srv;
	boost::asio::deadline_timer m_check_tick;
	boost::shared_mutex m_mutex;
	unsigned short m_next_session;

	std::list<socket_session_ptr> m_sessions;

	//void check_connection();
};

#endif // !SESSION_MANAGER_H__