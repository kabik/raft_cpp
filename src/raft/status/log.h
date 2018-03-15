#ifndef LOG_H
#define LOG_H

#include <string>
#include <vector>

#include "../../fileHandler.h"
#include "entry.cc"

using std::string;
using std::vector;

class Log : public FileHandler {
private:
	vector<entry* > _log;

public:
	Log(string storageDirectoryName);

	int size();
	int lastLogIndex();
	int lastLogTerm();
	entry get(int index);
	void add(int term, string command);
};

#include "log.cc"
#endif //LOG_H
