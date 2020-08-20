#include "HistoryItemDBWriter.h"
#include "HistoryMysqlDb.h"
#include <boost\thread\lock_guard.hpp>
#include "HistoryItem.h"

extern HistoryMysqlDb history_db;

std::list<HistoryItem> HistoryItemDBWriter::bufferlist;
std::list<HistoryItem> HistoryItemDBWriter::bufferlist_external;

boost::mutex HistoryItemDBWriter::mymutex;
bool HistoryItemDBWriter::threadrunning = false;
boost::interprocess::interprocess_semaphore HistoryItemDBWriter::mysemaphore(1);
HistoryItemDBWriter::HistoryItemDBWriter()
{
}


HistoryItemDBWriter::~HistoryItemDBWriter()
{
}

void HistoryItemDBWriter::strategy_run()
{
	while (threadrunning) {
		mysemaphore.wait();

		{
			boost::lock_guard<boost::mutex> guard(mymutex);
			for each (const HistoryItem& item in bufferlist_external)
			{
				bufferlist.push_back(item);
			}
			bufferlist_external.clear();
		}
		for each (const HistoryItem &item in bufferlist)
		{
			if (history_db.HistoryItemExist(item)) {
				history_db.UpdateHistoryItem(item);
			}
			else {
				history_db.SaveHistoryItem(item);
			}
		}

		bufferlist.clear();
		//std::this_thread::sleep_for(std::chrono::milliseconds(400));//1秒基本上够了
	}
}

void HistoryItemDBWriter::StartDaemonWriter()
{
	threadrunning = true;
	std::thread t(strategy_run);
	t.detach();

}
void HistoryItemDBWriter::StopDaemonWriter()
{
	threadrunning = false;
	mysemaphore.post();
}

void HistoryItemDBWriter::StoreHistoryItem(HistoryItem &item)
{
	{
		boost::lock_guard<boost::mutex> guard(mymutex);
		bufferlist_external.push_back(item);
	}
	mysemaphore.post();
}