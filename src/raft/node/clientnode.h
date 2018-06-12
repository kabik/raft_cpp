#ifndef CLIET_NODE_H
#define CLIET_NODE_H

#include "node.h"

class ClientNode : public Node {
private:
	int id;
	int nextIndex;
	int matchIndex;
	int sentIndex;
	bool isme;
	bool votedForMe;
	bool rvrpcSent;     // requestVoteRPC

public:
	ClientNode(string* hostname, int port);

	int getID();
	void setID(int id);

	int  getNextIndex();
	void setNextIndex(int nextIndex);

	int  getMatchIndex();
	void setMatchIndex(int matchIndex);

	int  getSentIndex();
	void setSentIndex(int sentIndex);

	bool isMe();
	void setIsMe(bool isme);

	bool hasVotedForMe();
	void setVotedForMe(bool votedForMe);

	bool IhaveSentRequestVoteRPC();
	void setRequestVoteRPCSent(bool rvrpcSent);
};

#include "clientnode.cc"
#endif //CLIENT_NODE_H
