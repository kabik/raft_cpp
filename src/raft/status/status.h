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
	int commitIndex;
	int lastApplied;

	void createDirectory();

public:
	Status(string storageDirectoryName);

	string getStorageDirectoryName();

	int getCurrentTerm();
	void incrementCurrentTerm();

	int getVotedFor();
	void setVotedFor(int node_id);

	int getCommitIndex();
	void setCommitIndex(int commitIndex);

	int getLastApplied();
	void setLastApplied(int lastApplied);
};

#include "status.cc"
#endif //STATUS_H
