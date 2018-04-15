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
#include "status/entry.cc";
#include "node/raftnode.h"
#include "state.h"
#include "constant.h"
#include "rpc.cc"
#include "../functions.cc"


using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::this_thread::sleep_for;
using std::chrono::milliseconds;
using std::chrono::microseconds;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;

static void* work(void* args);
static void sendMessage(Raft* raft, RaftNode* rNode, char* msg, int length);
static void connect2raftnode(Raft* raft, RaftNode* rNode);
static void startWorkerThread(Raft* raft, RaftNode* rNode);

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

// to use receive thread
void Raft::receive() {
	// === bind ===
	int listenSocket;
	struct sockaddr_in addr, client;
	const int yes = 1;

	if ((listenSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	int listenPort = this->getRaftNodeById(this->getMe())->getListenPort();

	addr.sin_family = AF_INET;
	addr.sin_port = htons(listenPort);
	addr.sin_addr.s_addr = INADDR_ANY;

	setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));

	if (bind(listenSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("bind");
		exit(1);
	}


	// === listen ===
	if (listen(listenSocket, 30) < 0) {
		perror("listen");
		close(listenSocket);
		exit(1);
	} else {
		cout << "Listening in Port " << listenPort << "." << endl;
	}


	// === accept ===
	while(1) {
		int len = sizeof(client);
		int sock;
		if ((sock = accept(listenSocket, (struct sockaddr*)&client, (unsigned int*)&len)) < 0) {
			perror("accept");
			exit(1);
		}
		for (RaftNode* rNode : *this->getRaftNodes()) {
			if (!rNode->isMe() && inet_ntoa(client.sin_addr) == rNode->getHostname() && rNode->getReceiveSock() < 0) {
				rNode->setReceiveSock(sock);
				startWorkerThread(this, rNode);

				cout << rNode->getHostname() << " connected from it. (sock=" << sock << ")\n";
			}
		}
		// if client
	}
}

// to use timer thread
void Raft::timer() {
	this->resetStartTime();

	while(1) {
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

void Raft::cli() {
	char cKind[COMMAND_KIND_LENGTH];
	char key[KEY_LENGTH];
	int value;

	while (1) {
		cout << "> ";
		string s;
		getline(cin, s);

		if (this->getStatus()->isLeader()) {
			vector<string> vec = split(s, ' ');
			vec[0].copy(cKind, COMMAND_KIND_LENGTH);
			vec[1].copy(key  , KEY_LENGTH);
			if (vec.size() > 2) { value = stoi(vec[2]); }

			// add to log
			char command[COMMAND_STR_LENGTH];
			if (vec.size() > 2) {
				sprintf(command, "%s %s %d", cKind, key, value);
			} else {
				sprintf(command, "%s %s"   , cKind, key);
			}
			int cTerm = this->getStatus()->getCurrentTerm();
			this->getStatus()->getLog()->add(cTerm, command);

			cout << "command = " << command << endl;
		} else {
			cout << "I am NOT LEADER!\n";
		}
	}
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

int Raft::getVote() {
	return this->vote;
}
void Raft::setVote(int vote) {
	_mtx.lock();
	this->vote = vote;
	_mtx.unlock();
}

int Raft::getLeaderTerm() {
	return this->leaderTerm;
}
void Raft::setLeaderTerm(int leaderTerm) {
	this->leaderTerm = leaderTerm;
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
			sendMessage(this, rNode, msg, APPEND_ENTRIES_RPC_LENGTH);
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
			sendMessage(this, rNode, msg, REQUEST_VOTE_RPC_LENGTH);
		}
	}
}

static void appendEntriesRecieved(Raft* raft, RaftNode* rNode, char* msg) {
	raft->resetStartTime();

	// prepare
	bool grant = true;
	int currentTerm = raft->getStatus()->getCurrentTerm();
	Log* log = raft->getStatus()->getLog();

	append_entries_rpc* arpc = (append_entries_rpc*)malloc(sizeof(append_entries_rpc));
	str2arpc(msg, arpc);

	// check valid appendEntriesRPC or not
	if (currentTerm > arpc->term ||
		!log->match(arpc->prevLogIndex, arpc->prevLogTerm)) {
		grant = false;
	}
	if (raft->getStatus()->isCandidate() && currentTerm <= arpc->term) {
		// become Follower
		raft->getStatus()->becomeFollower();

		// vote for the sender
		raft->setVote(0);
		raft->getStatus()->setVotedFor(arpc->leaderId);

		raft->setLeaderTerm(arpc->term);
	}

	// do if grant == true
	if (grant) {
		while(raft->getStatus()->getCurrentTerm() < arpc->term) {
			raft->getStatus()->incrementCurrentTerm();
		}
		currentTerm = raft->getStatus()->getCurrentTerm();

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
	sendMessage(raft, rNode, str, RESPONSE_APPEND_ENTRIES_LENGTH);

	free(rae);
	free(arpc);
}
static void requestVoteReceived(Raft* raft, RaftNode* rNode, char* msg) {
	raft->resetStartTime();

	// prepare
	bool grant = true;
	int currentTerm = raft->getStatus()->getCurrentTerm();

	request_vote_rpc* rrpc = (request_vote_rpc*)malloc(sizeof(request_vote_rpc));
	str2rrpc(msg, rrpc);

	// check valid candidate or not
	Log* log = raft->getStatus()->getLog();

	if (rrpc->term < currentTerm) {
		grant = false;
	}
	if ((raft->getStatus()->isLeader() || raft->getStatus()->isCandidate()) &&
		rrpc->term <= currentTerm) {
		grant = false;
	}
	if (rrpc->term <= raft->getLeaderTerm() && rrpc->candidateId != raft->getStatus()->getVotedFor()) {
		grant = false;
	}
	if (rrpc->lastLogTerm < log->lastLogTerm() ||
		(rrpc->lastLogTerm == log->lastLogTerm() && rrpc->lastLogIndex < log->lastLogIndex())) {
		grant = false;
	}

	if (grant) {
		raft->setLeaderTerm(rrpc->term);
		raft->getStatus()->setVotedFor(rrpc->candidateId);
		cout << "vote for " << rrpc->candidateId << " leaderTerm=" << raft->getLeaderTerm() << " votedFor=" << raft->getStatus()->getVotedFor() << endl;
	}

	// send response to the Candidate
	char str[RESPONSE_REQUEST_VOTE_LENGTH];
	response_request_vote* rrv = (response_request_vote*)malloc(sizeof(response_request_vote));
	rrvByFields(rrv, raft->getStatus()->getCurrentTerm(), grant);
	rrv2str(rrv, str);
	sendMessage(raft, rNode, str, RESPONSE_REQUEST_VOTE_LENGTH);

	free(rrpc);
	free(rrv);
}
static void responceAppendEntriesReceived(Raft* raft, RaftNode* rNode, char* msg) {
	response_append_entries* rae = (response_append_entries*)malloc(sizeof(response_append_entries));
	str2rae(msg, rae);

	free(rae);
}
static void responceRequestVoteReceived(Raft* raft, RaftNode* rNode, char* msg) {
	response_request_vote* rrv = (response_request_vote*)malloc(sizeof(response_request_vote));
	str2rrv(msg, rrv);

	if (rrv->success) {
		raft->setVote(raft->getVote() + 1);
	}
	if (raft->getVote() > raft->getConfig()->getNumberOfNodes() / 2) {
		// become Leader
		cout << "win the election at term " << raft->getStatus()->getCurrentTerm() << endl;
		raft->setVote(0);
		raft->getStatus()->becomeLeader();
	}

	free(rrv);
}

static void sendMessage(Raft* raft, RaftNode* rNode, char* msg, int length) {
	connect2raftnode(raft, rNode);
	rNode->send(msg, length);
}

static void connect2raftnode(Raft* raft, RaftNode* rNode) {
	// return if I already connect rNode
	if (rNode->getSendSock() > 0) {
		return;
	}

	// connect
	struct sockaddr_in server;

	int sock = socket(AF_INET, SOCK_STREAM, 0);

	server.sin_family = AF_INET;
	server.sin_port = htons(rNode->getListenPort());
	server.sin_addr.s_addr = inet_addr(rNode->getHostname().c_str());

	if ((connect(sock, (struct sockaddr *)&server, sizeof(server))) < 0) {
		perror("connect");
		exit(1);
	} else {
		rNode->setSendSock(sock);
	}
}

static void startWorkerThread(Raft* raft, RaftNode* rNode) {
	pthread_t worker;
	worker_args* wargs = (worker_args*)malloc(sizeof(worker_args));
	wargs->raft = raft;
	wargs->rNode = rNode;
	if (pthread_create( &worker, NULL, work, (void*)wargs ) < 0) {
		perror("pthread_create");
		exit(1);
	}
	rNode->setWorker(&worker);
	pthread_detach(worker);
}

// worker thread
static void* work(void* args) {
	worker_args* wargs = (worker_args*)args;

	Raft*     raft  = wargs->raft;
	RaftNode* rNode = wargs->rNode;

	char buf[BUFFER_SIZE];
	int sock = rNode->getReceiveSock();

	while(1) {
		memset(buf, 0, sizeof(buf));
		if (recv(sock, buf, sizeof(buf), MSG_WAITALL) < 0) {
			perror("recv");
			exit(1);
		}
		RPCKind rpcKind = discernRPC(buf);
		//cout << StrRPCKind(rpcKind) << " from " << rNode->getHostname().c_str() << ": [" << buf << "]\n";

		if        (rpcKind == RPC_KIND_APPEND_ENTRIES) {
			appendEntriesRecieved(raft, rNode, buf);

		} else if (rpcKind == RPC_KIND_REQUEST_VOTE) {
			requestVoteReceived(raft, rNode, buf);

		} else if (rpcKind == RPC_KIND_RESPONSE_APPEND_ENTRIES) {
			responceAppendEntriesReceived(raft, rNode, buf);

		} else if (rpcKind == RPC_KIND_RESPONSE_REQUEST_VOTE) {
			responceRequestVoteReceived(raft, rNode, buf);

		}
	}
}
