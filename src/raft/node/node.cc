#include "node.h"

Node::Node(string* hostname, int listenPort) {
	this->setHostname(*hostname);
	this->setListenPort(listenPort);
	this->sock = 0;
}

void Node::send(char* message, int length) {
	write(this->getSock(), message, length);
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
