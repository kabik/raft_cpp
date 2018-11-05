#include "clientnode.h"

ClientNode::ClientNode(string* hostname, int port) : Node(hostname, port) {
	this->lastIndex   = -1;
	this->commitIndex = -1;
}

int ClientNode::getLastIndex() {
	return this->lastIndex;
}
void ClientNode::setLastIndex(int lastIndex) {
	this->lastIndex = lastIndex;
}

int ClientNode::getCommitIndex() {
	return this->commitIndex;
}
void ClientNode::setCommitIndex(int commitIndex) {
	this->commitIndex = commitIndex;
}
