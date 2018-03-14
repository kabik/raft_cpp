#ifndef STATUS_H
#define STATUS_H

using std::string;

class SavedValue;
class Log;

class Status {
private:
	string* storageDirectoryName;
	Log* log;
	SavedValue* currentTerm;
	SavedValue* votedFor;

	void createDirectory();

public:
	Status(string storageDirectoryName);

	string getStorageDirectoryName();
};

#include "status.cc"
#endif //STATUS_H
