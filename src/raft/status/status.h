#ifndef STATUS_H
#define STATUS_H

#include <mutex>

#include "../state.h"

using std::string;

class SavedValue;
class Log;

class Status {
private:
	std::mutex _mtx;

	string* storageDirectoryName;
	Log* log;
	State state;
	SavedValue* currentTerm;
	SavedValue* votedFor;
	int commitIndex;
	int lastApplied;
	int timeouttime;

	void createDirectory();

public:
	Status(string storageDirectoryName);

	string getStorageDirectoryName();

	Log* getLog();

	State getState();
	void  setState(State state);
	bool isFollower();
	bool isCandidate();
	bool isLeader();
	void becomeFollower();
	void becomeCandidate();
	void becomeLeader();

	int  getCurrentTerm();
	void incrementCurrentTerm();

	int  getVotedFor();
	void setVotedFor(int node_id);

	int  getCommitIndex();
	void setCommitIndex(int commitIndex);

	int  getLastApplied();
	void setLastApplied(int lastApplied);

	int  getTimeoutTime();
	void setTimeoutTime(int timeouttime);
};

#include "status.cc"
#endif //STATUS_H
