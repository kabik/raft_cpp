#include <stdio.h>
#include <string.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <chrono>

#include "raft.h"
#include "../config.h"
#include "status/status.h"
#include "node/raftnode.h"
#include "state.h"
#include "constant.h"
#include "rpc.cc"
#include "../functions.cc"

using std::cout;
using std::cerr;
using std::endl;
using std::this_thread::sleep_for;
using std::chrono::milliseconds;
using std::chrono::microseconds;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;

Raft::Raft(char* configFileName) {
	this->config = new Config(configFileName);
	this->raftNodes = new vector<RaftNode*>;
	this->setRaftNodesByConfig();

	this->status = new Status(this->getConfig()->getStorageDirectoryName());
	this->status->setState(FOLLOWER);
	this->resetTimeoutTime();
}

void Raft::receive() {
	fd_set fds, readfds;
	char buf[2048];

	FD_ZERO(&readfds);

	int maxfd = 0;
	for (RaftNode* rNode : *this->getRaftNodes()) {
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
		for (RaftNode* rNode : *this->getRaftNodes()) {
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

void Raft::timer() {
	this->resetStartTime();

	while(1) {
		sleep_for(milliseconds(100));

		cout << StrState(this->getStatus()->getState())
			<< " term=" << this->getStatus()->getCurrentTerm()
			<< " duration=" << this->getDuration().count()
			<< endl;

		if (this->isTimeout()) {
			cout << "timeout in term " << this->getStatus()->getCurrentTerm() << endl;
			this->resetStartTime();
			if (this->getStatus()->isFollower() || this->getStatus()->isCandidate()) {
				candidacy();
			}
		} else {

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

	int listenPort = this->getRaftNodeById(this->getMe())->getListenPort();

	addr.sin_family = AF_INET;
	addr.sin_port = htons(listenPort);
	addr.sin_addr.s_addr = INADDR_ANY;
	bind(listenSocket, (struct sockaddr*)&addr, sizeof(addr));

	cout << "Listening in Port " << listenPort << "." << endl;

	while (1) {
		listen(listenSocket, 10);

		len = sizeof(client);
		sock = accept(listenSocket, (struct sockaddr*)&client, (unsigned int*)&len);
		for (RaftNode* rNode : *this->getRaftNodes()) {
			if (inet_ntoa(client.sin_addr) == rNode->getHostname() &&
				rNode->getSock() == 0) {
				rNode->setSock(sock);
			}
		}
	}

	close(listenSocket);
}

void Raft::connectOtherRaftNodes() {
	for (RaftNode* rNode : *this->getRaftNodes()) {
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

void Raft::resetTimeoutTime() {
	int tt = myrand(MIN_TIMEOUTTIME_MICROSECONDS, MAX_TIMEOUTTIME_MICROSECONDS);
	this->status->setTimeoutTime(tt);
}

bool Raft::isTimeout() {
	return this->getDuration().count() > this->getStatus()->getTimeoutTime();
}
microseconds Raft::getDuration() {
	return duration_cast<microseconds>(high_resolution_clock::now() - this->startTime);
}
void Raft::resetStartTime() {
	this->startTime = high_resolution_clock::now();
}

Config* Raft::getConfig() {
	return this->config;
}
Status* Raft::getStatus() {
	return this->status;
}

void Raft::setRaftNodesByConfig() {
	int cnt = 0;
	for (node_conf* nconf : this->config->getNodes()) {
		RaftNode* rNode = new RaftNode(nconf->hostname, nconf->port);
		rNode->setID(cnt);
		this->raftNodes->push_back(rNode);
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
			for (RaftNode* rNode : *this->getRaftNodes()) {
				if (strcmp(rNode->getHostname().c_str(), addrstr) == 0) {
					rNode->setIsMe(true);
					this->setMe(rNode->getID());
				}
			}
		}
	}
	for (RaftNode* rNode : *this->getRaftNodes()) {
		if (rNode->isMe()) {
			cout << "I am \"" << rNode->getHostname() << ":" << rNode->getListenPort() << "\"." << endl;
		}
	}

	freeifaddrs(ifa_list);
}

vector<RaftNode*>* Raft::getRaftNodes() {
	return this->raftNodes;
}
RaftNode* Raft::getRaftNodeById(int id) {
	return (*this->raftNodes)[id];
}

void Raft::setMe(int me) {
	this->me = me;
}
int Raft::getMe() {
	return this->me;
}

/* === private functions === */
void Raft::candidacy() {
	// become candidate
	this->getStatus()->becomeCandidate();

	// increment term
	this->getStatus()->incrementCurrentTerm();

	// vote for me
	this->getStatus()->setVotedFor(this->getMe());

	// send request vote rpcs
	for (RaftNode* rNode : *this->getRaftNodes()) {
		if (!rNode->isMe()) {
			char msg[REQUEST_VOTE_RPC_LENGTH];
			request_vote_rpc* rrpc = (request_vote_rpc*)malloc(sizeof(request_vote_rpc));
			rrpcByFields(
				rrpc,
				this->getStatus()->getCurrentTerm(),
				this->getMe(),
				this->getStatus()->getLog()->lastLogIndex(),
				this->getStatus()->getLog()->lastLogTerm()
			);
			rrpc2str(rrpc, msg);
			rNode->send(msg, REQUEST_VOTE_RPC_LENGTH);
		}
	}
}
