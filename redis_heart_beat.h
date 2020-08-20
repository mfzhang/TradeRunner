#ifndef __REDIS_HEART_BEAT_H__
#define __REDIS_HEART_BEAT_H__
#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp> 
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <algorithm>
#include "redisclient.h"
class redis_heart_beat
{
public:
	redis_heart_beat(RedisClient &_Redis, boost::asio::io_service &ioservice,int _seconds);
	~redis_heart_beat();
	void run(std::string _key);
private:
	std::string key;
	RedisClient &redis;
	int seconds;
	boost::asio::deadline_timer heartBeatTime;

	/*
	* ß\ÐÐ î‘BŒ‘Èëredis
	*/
	void handle_heartbeat(const boost::system::error_code & error);
};

#endif