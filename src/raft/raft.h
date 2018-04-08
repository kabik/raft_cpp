#ifndef RAFT_H
#define RAFT_H

#include <vector>
#include <chrono>

using std::vector;
using std::chrono::high_resolution_clock;
using std::chrono::microseconds;

class Config;
class Status;
class Raft;
class RaftNode;

typedef struct _worker_args {
	Raft* raft;
	RaftNode* rNode;
} worker_args;

class Raft {
private:
	Config* config;
	Status* status;
	vector<RaftNode*>* raftNodes;
	int me;
	high_resolution_clock::time_point startTime;

	int leaderTerm;
	int vote;

	/* === private functions === */
	void setRaftNodesByConfig();

	void appendEntriesRPC();
	void candidacy();

public:
	Raft(char* configFileName);

	void receive();
	void timer();
	void resetTimeoutTime();

	bool isTimeout();
	microseconds getDuration();
	void resetStartTime();

	Config* getConfig();
	Status* getStatus();

	vector<RaftNode*>* getRaftNodes();
	RaftNode* getRaftNodeById(int id);

	void setMe(int me);
	int getMe();

	int getLeaderTerm();
	void setLeaderTerm(int leaderTerm);

	int getVote();
	void setVote(int vote);
};

#include "raft.cc"
#endif //RAFT_H
