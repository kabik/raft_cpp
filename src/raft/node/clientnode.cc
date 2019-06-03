#include "clientnode.h"

ClientNode::ClientNode(string* hostname, int port) : Node(hostname, port) {
	this->lastIndex   = -1;
	this->commitIndex = -1;
	this->lastCommandId      = -1;
	this->committedCommandId = -1;
	this->needReadRequest = false;
	this->readRPCID = -1;
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

int ClientNode::getLastCommandId() {
	return this->lastCommandId;
}
void ClientNode::setLastCommandId(int lastCommandId) {
	this->lastCommandId = lastCommandId;
}

int ClientNode::getCommittedCommandId() {
	return this->committedCommandId;
}
void ClientNode::setCommittedCommandId(int committedCommandId) {
	this->committedCommandId = committedCommandId;
}

void ClientNode::setReadRPCID(int rpcid) {
	_mtx.lock();
	this->readRPCID = rpcid;
	_mtx.unlock();
}
int ClientNode::getReadRPCID() {
	int ret;
	_mtx.lock();
	ret = this->readRPCID;
	_mtx.unlock();
	return ret;
}
void ClientNode::setNeedReadRequest(bool b) {
	_mtx.lock();
	this->needReadRequest = b;
	_mtx.unlock();
}
bool ClientNode::getNeedReadRequest() {
	bool ret;
	_mtx.lock();
	ret = this->needReadRequest;
	_mtx.unlock();
	return ret;
}
int ClientNode::getReadGrantsNum(int size) {
	int num = 1; // myself
	_mtx.lock();
	for (bool isGranted : this->readGrants) {
		if (isGranted) { num++; }
	}
	_mtx.unlock();

	return num;
}
void ClientNode::grant(int raftNodeId) {
	_mtx.lock();
	readGrants[raftNodeId] = true;
	_mtx.unlock();
}
void ClientNode::resetReadGrants(int size) {
	_mtx.lock();
	for (int i = 0; i < size; i++) {
		if (i < readGrants.size()) {
			readGrants[i] = false;
		} else {
			readGrants.push_back(0);
		}
	}
	_mtx.unlock();
}
