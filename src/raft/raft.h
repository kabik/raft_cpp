#ifndef RAFT_H
#define RAFT_H

#include <vector>
#include <chrono>

using std::vector;
using std::chrono::high_resolution_clock;
using std::chrono::microseconds;

class Config;
class Status;
class RaftNode;

class Raft {
private:
	Config* config;
	Status* status;
	vector<RaftNode*>* raftNodes;
	int me;
	high_resolution_clock::time_point startTime;

	void candidacy();

public:
	Raft(char* configFileName);

	void receive();
	void timer();
	void listenTCP();
	void connectOtherRaftNodes();
	void resetTimeoutTime();

	bool isTimeout();
	microseconds getDuration();
	void resetStartTime();

	Config* getConfig();
	Status* getStatus();

	void setRaftNodesByConfig();
	vector<RaftNode*>* getRaftNodes();
	RaftNode* getRaftNodeById(int id);

	void setMe(int me);
	int getMe();
};

#include "raft.cc"
#endif //RAFT_H
