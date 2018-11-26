#include "node.h"
#include "../../functions.cc"

Node::Node(string* hostname, int listenPort) {
	this->setHostname(*hostname);
	this->setListenPort(listenPort);
	this->receiveSock = -1;
	this->sendSock = -1;
}

int Node::getID() {
	return this->id;
}
void Node::setID(int id) {
	this->id = id;
}

void Node::_send(char* message, int length) {
	//cout << "send message to " << this->getHostname() << " [" << message << "] sock=" << sendSock << endl;
	if (S_SEND(this->getSendSock(), message, length, 0) < 0) {
		perror("write");
		this->sendSock = -1;
		cout << "    node is " << this->getHostname() << endl;
	}
}

string Node::getHostname() {
	return *this->hostname;
}
void Node::setHostname(string hostname) {
	this->hostname = new string(hostname);
}

int Node::getListenPort() {
	return this->listenPort;
}
void Node::setListenPort(int listenPort) {
	this->listenPort = listenPort;
}

int Node::getReceiveSock() {
	return this->receiveSock;
}
void Node::setReceiveSock(int sock) {
	this->receiveSock = sock;
}
int Node::getSendSock() {
	return this->sendSock;
}
void Node::setSendSock(int sock) {
	this->sendSock = sock;
}

pthread_t* Node::getWorker() {
	return this->worker;
}
void Node::setWorker(pthread_t* worker) {
	this->worker = worker;
}
