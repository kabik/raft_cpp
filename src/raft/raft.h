#ifndef RAFT_H
#define RAFT_H

#include <vector>
#include <map>
#include <chrono>
#include <mutex>

using std::vector;
using std::map;
using std::chrono::high_resolution_clock;
using std::chrono::microseconds;
using std::mutex;


class Config;
class Status;
class KVS;
class Raft;
class RaftNode;
class ClientNode;

typedef struct _worker_args {
	Raft* raft;
	RaftNode* rNode;
	ClientNode* cNode;
	bool isClient;
} worker_args;

class Raft {
private:
	mutex _mtx;

	Config* config;
	Status* status;
	KVS* kvs;
	vector<RaftNode*>* raftNodes;
	vector<ClientNode*>* clientNodes;
	int me;
	high_resolution_clock::time_point startTime;

	int leaderTerm;
	int vote;

	int commitCount;

	/* === private functions === */
	void setRaftNodesByConfig();

	void candidacy();

public:
	Raft(char* configFileName);

	void lock();
	void unlock();

	int incrementCommitCount();

	void receive();
	void timer();
	void resetTimeoutTime();
	void cli();

	bool isTimeout();
	microseconds getDuration();
	void resetStartTime();

	Config* getConfig();
	Status* getStatus();
	KVS* getKVS();

	vector<RaftNode*>* getRaftNodes();
	RaftNode* getRaftNodeById(int id);
	RaftNode* getLeader();

	vector<ClientNode*>* getClientNodes();
	void addClientNode(ClientNode* cNode);

	void setMe(int me);
	int getMe();

	int getLeaderTerm();
	void setLeaderTerm(int leaderTerm);

	int getVote();
	void setVote(int vote);

	void apply(int index);

	void sendAppendEntriesRPC(RaftNode* rNode, int rpcId, bool isHeartBeat, bool isRequestRead);
};

#include "raft.cc"
#endif //RAFT_H
