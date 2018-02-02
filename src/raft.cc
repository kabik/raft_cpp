#include <stdio.h>
#include <string.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "raft.h"
#include "config.h"
#include "state.h"
#include "node/raftnode.h"

using namespace std;

Raft::Raft() {}

void Raft::createConfig(char* configFileName) {
    this->config = new Config(configFileName);
}

Config* Raft::getConfig() {
    return this->config;
}

void Raft::setRaftNodesByConfig() {
    for (node_conf* nconf : this->config->getNodes()) {
        RaftNode* rNode = new RaftNode(nconf->hostname, nconf->port);
        this->raftNodes.push_back(rNode);
    }

    struct ifaddrs* ifa_list;
    struct ifaddrs* ifa;

    int n;
    char addrstr[256];

    n = getifaddrs(&ifa_list);
    if (n != 0) {
        exit(1);
    }

    for (ifa = ifa_list; ifa != NULL; ifa = ifa->ifa_next) {
        memset(addrstr, 0, sizeof(addrstr));

        if (ifa->ifa_addr->sa_family == AF_INET) {
            inet_ntop(AF_INET,
                &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr,
                addrstr, sizeof(addrstr));
            for (RaftNode* rNode : this->raftNodes) {
                if (strcmp(rNode->getHostname().c_str(), addrstr) == 0) {
                    rNode->setIsMe(true);
                }
            }
        }
    }
    for (RaftNode* rNode : this->raftNodes) {
        if (rNode->isMe()) {
            cout << "I am " << rNode->getHostname() << ":" << rNode->getListenPort() << "." << endl;
        }
    }

    freeifaddrs(ifa_list);
}

vector<RaftNode*> Raft::getRaftNodes() {
    return this->raftNodes;
}
