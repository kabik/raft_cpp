#ifndef LOG_H
#define LOG_H

#include <string>
#include <vector>
#include <mutex>

#include "../../fileHandler.h"
#include "entry.cc"

using std::string;
using std::vector;

class Log : public FileHandler {
private:
	std::mutex _mtx;
	vector<entry* > _log;
	int lastSyncedIndex;

public:
	Log(string storageDirectoryName);

	int size();
	int lastLogIndex();
	int lastLogTerm();
	int getTerm(int index);
	int getConnectionId(int index);
	bool match(int index, int term);
	entry* get(int index);
	void add(int term, int conn_id, const char command[COMMAND_STR_LENGTH]);
	void add(entry* entries[], int num);
	int getLastSyncedIndex();
	void sync();

	void printAll();
};

#include "log.cc"
#endif //LOG_H
