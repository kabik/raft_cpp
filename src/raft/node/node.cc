#include "node.h"

Node::Node(string* hostname, int listenPort) {
	this->setHostname(*hostname);
	this->setListenPort(listenPort);
	this->sock = 0;
}

void Node::send(char* message, int length) {
	int sock = this->getSock();

	if (sock == 0) {
		struct sockaddr_in server;

		sock = socket(AF_INET, SOCK_STREAM, 0);

		server.sin_family = AF_INET;
		server.sin_port = htons(this->getListenPort());
		server.sin_addr.s_addr = inet_addr(this->getHostname().c_str());

		if ((connect(sock, (struct sockaddr *)&server, sizeof(server))) < 0) {
			perror("connect");
			exit(1);
		} else {
			this->setSock(sock);
			cout << this->getHostname() << " connected from me. (sock=" << sock << ")\n";
		}
	}

	cout << "send message to " << this->getHostname() << " [" << message << "]\n";

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
