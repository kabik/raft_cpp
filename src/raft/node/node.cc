#include "node.h"

Node::Node(string* hostname, int listenPort) {
	this->setHostname(*hostname);
	this->setListenPort(listenPort);
	this->sock = 0;
}

void Node::send(char* message, int length) {
	//cout << "send message to " << this->getHostname() << " [" << message << "]\n";
	if (write(this->getSock(), message, length) < 0) {
		perror("write");
		exit(1);
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

int Node::getSock() {
	return this->sock;
}
void Node::setSock(int sock) {
	this->sock = sock;
}

pthread_t* Node::getWorker() {
	return this->worker;
}
void Node::setWorker(pthread_t* worker) {
	this->worker = worker;
}
