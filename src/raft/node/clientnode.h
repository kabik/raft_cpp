#ifndef CLIET_NODE_H
#define CLIET_NODE_H

#include "node.h"

class ClientNode : public Node {
private:
	int lastIndex;
	int commitIndex;

public:
	ClientNode(string* hostname, int port);

	int getLastIndex();
	void setLastIndex(int lastIndex);

	int getCommitIndex();
	void setCommitIndex(int commitIndex);
};

#include "clientnode.cc"
#endif //CLIENT_NODE_H
