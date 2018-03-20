#include <stdio.h>
#include <string.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "raft.h"
#include "../config.h"
#include "status/status.h"
#include "node/raftnode.h"
#include "state.h"
#include "constant.h"

using std::cout;
using std::cerr;
using std::endl;

Raft::Raft(char* configFileName) {
	this->config = new Config(configFileName);
	this->setRaftNodesByConfig();
	this->status = new Status(this->getConfig()->getStorageDirectoryName());
}

void Raft::receive() {
	fd_set fds, readfds;
	char buf[2048];

	FD_ZERO(&readfds);

	int maxfd = 0;
	for (RaftNode* rNode : this->getRaftNodes()) {
		if (!rNode->isMe()) {
			if (rNode->getSock() > maxfd) {
				maxfd = rNode->getSock();
			}
			FD_SET(rNode->getSock(), &readfds);
		}
	}

	while(1) {
		memcpy(&fds, &readfds, sizeof(fd_set));
		select(maxfd+1, &fds, NULL, NULL, NULL);
		for (RaftNode* rNode : this->getRaftNodes()) {
			int sock = rNode->getSock();
			if (!rNode->isMe() && FD_ISSET(sock, &fds)) {
				memset(buf, 0, sizeof(buf));
				recv(sock, buf, sizeof(buf), 0);
				if (buf[0] != 0) {
					printf("sock=%d %s: [%s]\n", sock, rNode->getHostname().c_str(), buf);
				}
			}
		}
	}
}

void Raft::listenTCP() {
	int listenSocket;
	struct sockaddr_in addr;
	struct sockaddr_in client;
	int len;
	int sock;

	listenSocket = socket(AF_INET, SOCK_STREAM, 0);

	int listenPort = this->getRaftNodes()[this->getMe()]->getListenPort();

	addr.sin_family = AF_INET;
	addr.sin_port = htons(listenPort);
	addr.sin_addr.s_addr = INADDR_ANY;
	bind(listenSocket, (struct sockaddr*)&addr, sizeof(addr));

	cout << "Listening in Port " << listenPort << "." << endl;

	while (1) {
		listen(listenSocket, 10);

		len = sizeof(client);
		sock = accept(listenSocket, (struct sockaddr*)&client, (unsigned int*)&len);
		for (RaftNode* rNode : this->getRaftNodes()) {
			if (inet_ntoa(client.sin_addr) == rNode->getHostname() &&
				rNode->getSock() == 0) {
				rNode->setSock(sock);
			}
		}
	}

	close(listenSocket);
}

void Raft::connectOtherRaftNodes() {
	for (RaftNode* rNode : this->getRaftNodes()) {
		if (!rNode->isMe() && rNode->getSock() == 0) {
			struct sockaddr_in server;
			int sock;

			sock = socket(AF_INET, SOCK_STREAM, 0);

			server.sin_family = AF_INET;
			server.sin_port = htons(rNode->getListenPort());
			server.sin_addr.s_addr = inet_addr(rNode->getHostname().c_str());

			cout << "Connecting to " << rNode->getHostname() << ":" << rNode->getListenPort() << endl;

			connect(sock, (struct sockaddr *)&server, sizeof(server));

			rNode->setSock(sock);
		}
	}
}

Config* Raft::getConfig() {
	return this->config;
}

void Raft::setRaftNodesByConfig() {
	int cnt = 0;
	for (node_conf* nconf : this->config->getNodes()) {
		RaftNode* rNode = new RaftNode(nconf->hostname, nconf->port);
		rNode->setID(cnt);
		this->raftNodes.push_back(rNode);
		cnt++;
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
					this->setMe(rNode->getID());
				}
			}
		}
	}
	for (RaftNode* rNode : this->raftNodes) {
		if (rNode->isMe()) {
			cout << "I am \"" << rNode->getHostname() << ":" << rNode->getListenPort() << "\"." << endl;
		}
	}

	freeifaddrs(ifa_list);
}

vector<RaftNode*> Raft::getRaftNodes() {
	return this->raftNodes;
}

void Raft::setMe(int me) {
	this->me = me;
}
int Raft::getMe() {
	return this->me;
}
