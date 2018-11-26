#ifndef NODE_H
#define NODE_H

#include<mutex>

class Node {
private:
	int id;
	string* hostname;
	int listenPort;
	int receiveSock;
	int sendSock;
	pthread_t* worker;

public:
	Node(string* hostname, int port);

	int getID();
	void setID(int id);

	void _send(char* message, int length);

	string getHostname();
	void setHostname(string hostname);

	int getListenPort();
	void setListenPort(int listenPort);

	int getReceiveSock();
	void setReceiveSock(int sock);
	int getSendSock();
	void setSendSock(int sock);

	pthread_t* getWorker();
	void setWorker(pthread_t* worker);
};

#include "node.cc"
#endif //NODE_H
