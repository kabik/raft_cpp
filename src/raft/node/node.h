#ifndef NODE_H
#define NODE_H

class Node {
private:
	string* hostname;
	int listenPort;
	int sock;
	pthread_t* worker;

public:
	Node(string* hostname, int port);

	void send(char* message, int length);

	string getHostname();
	void setHostname(string hostname);

	int getListenPort();
	void setListenPort(int listenPort);

	int getSock();
	void setSock(int sock);

	pthread_t* getWorker();
	void setWorker(pthread_t* worker);
};

#include "node.cc"
#endif //NODE_H
