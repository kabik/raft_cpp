#ifndef CLIET_NODE_H
#define CLIET_NODE_H

#include "node.h"

class ClientNode : public Node {
private:
	int lastIndex;
	int commitIndex;

	std::mutex _mtx;

	int lastCommandId;
	int committedCommandId;

	bool needReadRequest;
	int readRPCID;
	std::vector<bool> readGrants;


public:
	ClientNode(string* hostname, int port);

	int getLastIndex();
	void setLastIndex(int lastIndex);

	int getCommitIndex();
	void setCommitIndex(int commitIndex);

	int getLastCommandId();
	void setLastCommandId(int lastCommandId);

	int getCommittedCommandId();
	void setCommittedCommandId(int committedCommandId);

	void setReadRPCID(int rpcid);
	int getReadRPCID();
	void setNeedReadRequest(bool b);
	bool getNeedReadRequest();
	int getReadGrantsNum(int size);
	void grant(int raftNodeId);
	void resetReadGrants(int size);
};

#include "clientnode.cc"
#endif //CLIENT_NODE_H
