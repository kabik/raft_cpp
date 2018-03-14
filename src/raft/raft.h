#ifndef RAFT_H
#define RAFT_H

#include <vector>

using std::vector;

class Config;
class Status;
class RaftNode;

class Raft {
private:
	Config* config;
	Status* status;
	vector<RaftNode*> raftNodes;
	int me;

public:
	Raft(char* configFileName);

	void receive();
	void listenTCP();
	void connectOtherRaftNodes();

	//void createConfig(char* configFileName);
	Config* getConfig();

	//void setStatus(Status* status);
	Status* getStatus();

	void setRaftNodesByConfig();
	vector<RaftNode*> getRaftNodes();

	void setMe(int me);
	int getMe();
};

#include "raft.cc"
#endif //RAFT_H
