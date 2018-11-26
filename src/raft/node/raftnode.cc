#include "raftnode.h"

RaftNode::RaftNode(string* hostname, int port) : Node(hostname, port) {
	this->nextIndex  = 0;
	this->matchIndex = -1;
	this->sentIndex  = -1;
	this->votedForMe = false;
	this->rvrpcSent  = false;
	this->isme       = false;
}

int RaftNode::getNextIndex() {
	return this->nextIndex;
}
void RaftNode::setNextIndex(int nextIndex) {
	this->nextIndex = nextIndex;
}

int RaftNode::getMatchIndex() {
	return this->matchIndex;
}
void RaftNode::setMatchIndex(int matchIndex) {
	this->matchIndex = matchIndex;
}

int RaftNode::getSentIndex() {
	return this->sentIndex;
}
void RaftNode::setSentIndex(int sentIndex) {
	this->sentIndex = sentIndex;
}

bool RaftNode::isMe() {
	return this->isme;
}
void RaftNode::setIsMe(bool isme) {
	this->isme = isme;
}

bool RaftNode::hasVotedForMe() {
	return this->votedForMe;
}
void RaftNode::setVotedForMe(bool votedForMe) {
	this->votedForMe = votedForMe;
}

bool RaftNode::IhaveSentRequestVoteRPC() {
	return this->rvrpcSent;
}
void RaftNode::setRequestVoteRPCSent(bool rvrpcSent) {
	this->rvrpcSent = rvrpcSent;
}
