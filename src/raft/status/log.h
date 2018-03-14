#ifndef LOG_H
#define LOG_H

#include "../../fileHandler.h"

class Log : public FileHandler {
private:

public:
	Log(string storageDirectoryName);
};

#include "log.cc"
#endif //LOG_H
