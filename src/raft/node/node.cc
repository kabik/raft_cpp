#include "node.h"

Node::Node(string* hostname, int listenPort) {
    this->setHostname(*hostname);
    this->setListenPort(listenPort);
}

void Node::send(string message) {
}

string Node::receive() {
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
