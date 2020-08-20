#ifndef HISTORYITEM_DBWRITER_H__
#define HISTORYITEM_DBWRITER_H__

#include <list>
#include <mutex>
#include <boost\thread\mutex.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
/*
全部使用静态方法就行
*/
class HistoryItem;
class HistoryItemDBWriter
{
public:
	HistoryItemDBWriter();
	~HistoryItemDBWriter();
	static void StartDaemonWriter();
	static void StopDaemonWriter();

	static void StoreHistoryItem(HistoryItem &item);
private:
	static std::list<HistoryItem> bufferlist;
	static std::list<HistoryItem> bufferlist_external;

	static boost::mutex mymutex;
	static bool threadrunning;
	static void strategy_run();

	static boost::interprocess::interprocess_semaphore mysemaphore;
};

#endif // !HISTORYITEM_DBWRITER_H__