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

public:
	Log(string storageDirectoryName);

	int size();
	int lastLogIndex();
	int lastLogTerm();
	bool match(int index, int term);
	entry* get(int index);
	void add(int term, char command[COMMAND_STR_LENGTH]);
};

#include "log.cc"
#endif //LOG_H
