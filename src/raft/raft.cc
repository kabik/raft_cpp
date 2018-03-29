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

	// others
	this->leaderTerm = this->status->getCurrentTerm();
	this->vote = 0;
}

void Raft::receive() {
	fd_set fds, readfds;
	char buf[BUFFER_SIZE];

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
					RPCKind rpcKind = discernRPC(buf);
					cout << StrRPCKind(rpcKind) << " from " << rNode->getHostname().c_str() << ": [" << buf << "]\n";
					if        (rpcKind == RPC_KIND_APPEND_ENTRIES) {
						appendEntriesRecieved(rNode, buf);
					} else if (rpcKind == RPC_KIND_REQUEST_VOTE) {
						requestVoteReceived(rNode, buf);
					} else if (rpcKind == RPC_KIND_RESPONSE_APPEND_ENTRIES) {
						responceAppendEntriesReceived(rNode, buf);
					} else if (rpcKind == RPC_KIND_RESPONSE_REQUEST_VOTE) {
						responceRequestVoteReceived(rNode, buf);
					}
				}
			}
		}
	}
}

void Raft::timer() {
	this->resetStartTime();

	while(1) {
		sleep_for(milliseconds(100));

		if (this->getStatus()->isLeader() && this->getDuration().count() > HEARTBEAT_INTERVAL) {
			//cout << "send appendEntriesRPC" << endl;
			this->resetStartTime();
			this->appendEntriesRPC();
		}

		if (this->isTimeout()) {
			cout << "timeout in term " << this->getStatus()->getCurrentTerm() << endl;
			this->resetStartTime();
			if (this->getStatus()->isFollower() || this->getStatus()->isCandidate()) {
				cout << "candidacy" << endl;
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
			if (inet_ntoa(client.sin_addr) == rNode->getHostname() && rNode->getSock() == 0) {
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
void Raft::appendEntriesRPC() {
	// send heartbeat
	char msg[APPEND_ENTRIES_RPC_LENGTH];
	append_entries_rpc* arpc = (append_entries_rpc*)malloc(sizeof(append_entries_rpc));
	arpcByFields(
		arpc,
		this->getStatus()->getCurrentTerm(),
		this->getMe(),
		this->getStatus()->getLog()->lastLogIndex(),
		this->getStatus()->getLog()->lastLogTerm(),
		this->getStatus()->getCommitIndex(),
		""
	);
	arpc2str(arpc, msg);
	for (RaftNode* rNode : *this->getRaftNodes()) {
		if (!rNode->isMe()) {
			rNode->send(msg, APPEND_ENTRIES_RPC_LENGTH);
		}
	}

	free(arpc);
}

void Raft::candidacy() {
	this->resetTimeoutTime();

	// become candidate
	this->getStatus()->becomeCandidate();

	// increment term
	this->getStatus()->incrementCurrentTerm();

	// vote for me
	this->getStatus()->setVotedFor(this->getMe());
	this->vote = 1;

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
			free(rrpc);
			rNode->send(msg, REQUEST_VOTE_RPC_LENGTH);
		}
	}
}

void Raft::appendEntriesRecieved(RaftNode* rNode, char* msg) {
	this->resetStartTime();

	// prepare
	bool grant = true;
	int currentTerm = this->getStatus()->getCurrentTerm();
	Log* log = this->getStatus()->getLog();

	append_entries_rpc* arpc = (append_entries_rpc*)malloc(sizeof(append_entries_rpc));
	str2arpc(msg, arpc);

	// check valid appendEntriesRPC or not
	if (currentTerm > arpc->term ||
		!log->match(arpc->prevLogIndex, arpc->prevLogIndex)) {
		grant = false;
	}
	if (this->getStatus()->isCandidate() && currentTerm <= arpc->term) {
		// become Follower
		this->getStatus()->becomeFollower();

		// vote for the sender
		vote = 0;
		this->getStatus()->setVotedFor(arpc->leaderId);
		while(this->getStatus()->getCurrentTerm() < arpc->term) {
			this->getStatus()->incrementCurrentTerm();
		}

		currentTerm = this->getStatus()->getCurrentTerm();
		leaderTerm = arpc->term;
	}

	// do if grant == true
	if (grant) {
		if (!log->match(arpc->prevLogIndex, arpc->prevLogTerm)) {
			// delete log[index] : index >= arpc->prevLogIndex
		}
		// push entries to log

		// update commitIndex
	}

	// send response to the Leader
	char str[RESPONSE_APPEND_ENTRIES_LENGTH];
	response_append_entries* rae = (response_append_entries*)malloc(sizeof(response_append_entries));
	raeByFields(rae, currentTerm, grant);
	rae2str(rae, str);
	rNode->send(str, RESPONSE_APPEND_ENTRIES_LENGTH);

	free(rae);
	free(arpc);
}
void Raft::requestVoteReceived(RaftNode* rNode, char* msg) {
	this->resetStartTime();

	// prepare
	bool grant = true;
	int currentTerm = this->getStatus()->getCurrentTerm();

	request_vote_rpc* rrpc = (request_vote_rpc*)malloc(sizeof(request_vote_rpc));
	str2rrpc(msg, rrpc);

	cout << "rrpc->term=" << rrpc->term << endl;

	// check valid candidate or not
	if (rrpc->term < currentTerm) {
		grant = false;
	}
	if (rrpc->term == this->leaderTerm && rrpc->candidateId != this->getStatus()->getVotedFor()) {
		grant = false;
	}
	Log* log = this->getStatus()->getLog();
	if (rrpc->lastLogTerm < log->lastLogTerm() ||
		(rrpc->lastLogTerm == log->lastLogTerm() && rrpc->lastLogIndex < log->lastLogIndex())) {
		grant = false;
	}

	if (grant) {
		this->leaderTerm = rrpc->term;
		this->getStatus()->setVotedFor(rrpc->candidateId);
		cout << "vote for " << rrpc->candidateId << " leaderTerm=" << this->leaderTerm << " votedFor=" << this->getStatus()->getVotedFor() << endl;
	}

	// send response to the Candidate
	char str[RESPONSE_REQUEST_VOTE_LENGTH];
	response_request_vote* rrv = (response_request_vote*)malloc(sizeof(response_request_vote));
	rrvByFields(rrv, this->getStatus()->getCurrentTerm(), grant);
	rrv2str(rrv, str);
	rNode->send(str, RESPONSE_REQUEST_VOTE_LENGTH);

	free(rrpc);
	free(rrv);
}
void Raft::responceAppendEntriesReceived(RaftNode* rNode, char* msg) {
	response_append_entries* rae = (response_append_entries*)malloc(sizeof(response_append_entries));
	str2rae(msg, rae);
}
void Raft::responceRequestVoteReceived(RaftNode* rNode, char* msg) {
	response_request_vote* rrv = (response_request_vote*)malloc(sizeof(response_request_vote));
	str2rrv(msg, rrv);

	if (rrv->success) {
		vote++;
	}
	if (vote > this->getConfig()->getNumberOfNodes() / 2) {
		// become Leader
		cout << "win the election at term " << this->getStatus()->getCurrentTerm() << endl;
		vote = 0;
		this->getStatus()->becomeLeader();
		this->getStatus()->incrementCurrentTerm();
	}
}
