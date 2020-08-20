#include "redis_heart_beat.h"
#include "my_utils.h"
redis_heart_beat::redis_heart_beat(RedisClient &_Redis, boost::asio::io_service &ioservice,int _sec):redis(_Redis),seconds(_sec), heartBeatTime(ioservice)
{
}


redis_heart_beat::~redis_heart_beat()
{
}


void redis_heart_beat::run(std::string _key)
{
	key = _key;
	heartBeatTime.expires_from_now(boost::posix_time::seconds(seconds));
	heartBeatTime.async_wait(boost::bind(&redis_heart_beat::handle_heartbeat,
		this,
		boost::asio::placeholders::error));
}


/*
* ß\ÐÐ î‘BŒ‘Èëredis
*/
void redis_heart_beat::handle_heartbeat(const boost::system::error_code & error)
{
	if (redis.isConnected())
	{
		
		RedisBuffer key(key.c_str());
		std::string strValue = boost::str(boost::format("%1%") % my_utils::get_local_time());
		RedisBuffer value(strValue.c_str());
		redis.command("SET", key, value);
	}
	heartBeatTime.expires_from_now(boost::posix_time::seconds(seconds));
	heartBeatTime.async_wait(boost::bind(&redis_heart_beat::handle_heartbeat,
		this,
		boost::asio::placeholders::error));
}