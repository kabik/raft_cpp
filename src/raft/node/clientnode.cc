#include "clientnode.h"

ClientNode::ClientNode(string* hostname, int port) : Node(hostname, port) {
	this->nextIndex  = 0;
	this->matchIndex = -1;
	this->sentIndex  = -1;
	this->votedForMe = false;
	this->rvrpcSent  = false;
	this->isme       = false;
}
