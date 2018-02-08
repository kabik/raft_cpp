#ifndef NODE_H
#define NODE_H

class Node {
private:
    string* hostname;
    int listenPort;
    int sock;

public:
    Node(string* hostname, int port);

    void send(string message);
    string receive();

    string getHostname();
    void setHostname(string hostname);

    int getListenPort();
    void setListenPort(int listenPort);
};

#include "node.cc"
#endif //NODE_H
