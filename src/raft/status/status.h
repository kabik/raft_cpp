#ifndef STATUS_H
#define STATUS_H

#include <mutex>
#include <vector>

#include "../state.h"

using std::string;
using std::vector;

class SavedValue;
class Log;

class Status {
private:
	std::mutex _mtx;

	string* storageDirectoryName;
	Log* log;
	vector<int> savedCounts;
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
	int getSavedCount(int index);
	void incrementSavedCount(int index);

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
