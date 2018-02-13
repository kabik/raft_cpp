#ifndef RAFT_H
#define RAFT_H

#include <vector>

using std::vector;

class Config;
class RaftNode;

class Raft {
private:
	Config *config;
	vector<RaftNode*> raftNodes;

public:
	Raft();
	void createConfig(char* configFileName);
	Config* getConfig();
	void setRaftNodesByConfig();
	vector<RaftNode*> getRaftNodes();
};

#include "raft.cc"
#endif //RAFT_H
