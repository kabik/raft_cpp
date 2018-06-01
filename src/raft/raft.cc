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
#include "node/clientnode.h"
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
static void sendMessage(Raft* raft, ClientNode* cNode, char* msg, int length);
static void connect2raftnode(Raft* raft, RaftNode* rNode);
static void startWorkerThread(Raft* raft, RaftNode* rNode, ClientNode* cNode, bool isClient);

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
		bool isClient = true;
		for (RaftNode* rNode : *this->getRaftNodes()) {
			if (!rNode->isMe() && inet_ntoa(client.sin_addr) == rNode->getHostname() && rNode->getReceiveSock() < 0) {
				rNode->setReceiveSock(sock);
				startWorkerThread(this, rNode, NULL, false);
				isClient = false;

				cout << rNode->getHostname() << " connected from a Raft Node. (sock=" << sock << ")\n";
			}
		}
		// if client
		if (isClient) {
			string hostname(inet_ntoa(client.sin_addr));
			ClientNode *cNode = new ClientNode(&hostname, (int)ntohs(client.sin_port));
			cNode->setReceiveSock(sock);
			cNode->setSendSock(sock);
			startWorkerThread(this, NULL, cNode, true);

			cout << cNode->getHostname() << " connected from a Client. (sock=" << sock << ")\n";
		}
	}
}

// to use timer thread
void Raft::timer() {
	Log* log = this->getStatus()->getLog();
	this->resetStartTime();

	while(1) {
		// Leader
		if (this->getStatus()->isLeader()) {
			if (this->getDuration().count() > HEARTBEAT_INTERVAL) {
				this->resetStartTime();
				for (RaftNode* rNode : *this->getRaftNodes()) {
					if (!rNode->isMe()) {
						this->sendAppendEntriesRPC(rNode, true);
					}
				}
			} else {
				for (RaftNode* rNode : *this->getRaftNodes()) {
					if (!rNode->isMe()                                &&
					    log->lastLogIndex() >= rNode->getNextIndex()  &&
					    rNode->getNextIndex() > rNode->getSentIndex()
					) {
						this->sendAppendEntriesRPC(rNode, false);
					}
				}
			}
		}

		// All states
		if (this->isTimeout()) {
			cout << "timeout in term " << this->getStatus()->getCurrentTerm() << endl;
			this->resetStartTime();
			if (this->getStatus()->isFollower() || this->getStatus()->isCandidate()) {
				cout << "candidacy" << endl;
				candidacy();
			}
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
			vector<string> vec = split(s, COMMAND_DELIMITER);

			vec[0].copy(cKind, COMMAND_KIND_LENGTH);
			vec[1].copy(key  , KEY_LENGTH);
			if (vec.size() > 2) { value = stoi(vec[2]); }

			// add to log
			char command[COMMAND_STR_LENGTH];
			if (vec.size() > 2) {
				sprintf(command, "%s%c%s%c%d", cKind, COMMAND_DELIMITER, key, COMMAND_DELIMITER, value);
			} else {
				sprintf(command, "%s%c%s"    , cKind, COMMAND_DELIMITER, key);
			}
			int cTerm = this->getStatus()->getCurrentTerm();
			this->getStatus()->getLog()->add(cTerm, command);

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
RaftNode* Raft::getLeader() {
	return (*this->raftNodes)[this->status->getVotedFor()];
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
void Raft::sendAppendEntriesRPC(RaftNode* rNode, bool isHeartBeat) {
	Status* status = this->getStatus();

	// send heartbeat
	char msg[MESSAGE_SIZE];
	append_entries_rpc* arpc = (append_entries_rpc*)malloc(sizeof(append_entries_rpc));

	int nextIndex = rNode->getNextIndex();
	char entriesStr[ENTRIES_STR_LENGTH] = "";
	entriesStr[0] = '\0';
	if (!isHeartBeat) {
		entry* e = status->getLog()->get(nextIndex);
		entry2str(e, entriesStr);
	}

	arpcByFields(
		arpc,                                    // append_entries_rpc* arpc
		status->getCurrentTerm(),                // int term
		this->getMe(),                           // int leaderId
		nextIndex-1,                             // int prevLogIndex
		status->getLog()->getTerm(nextIndex-1),  // int prevLogTerm
		status->getCommitIndex(),                // int leaderCommit
		entriesStr                               // char entries[ENTRIES_STR_LENGTH]
	);
	arpc2str(arpc, msg);
	sendMessage(this, rNode, msg, MESSAGE_SIZE);
	if (!isHeartBeat) {
		rNode->setSentIndex(nextIndex);
	}

	free(arpc);
}

void Raft::candidacy() {
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
			char msg[MESSAGE_SIZE];
			request_vote_rpc* rrpc = (request_vote_rpc*)malloc(sizeof(request_vote_rpc));
			rrpcByFields(
				rrpc,                                         // request_vote_rpc* rrpc
				this->getStatus()->getCurrentTerm(),          // int term
				this->getMe(),                                // int candidateId
				this->getStatus()->getLog()->lastLogIndex(),  // int lastLogIndex
				this->getStatus()->getLog()->lastLogTerm()    // int lastLogTerm
			);
			rrpc2str(rrpc, msg);
			free(rrpc);
			sendMessage(this, rNode, msg, MESSAGE_SIZE);
		}
	}
	cout << "candidacy finished.\n";
}

static void appendEntriesRecieved(Raft* raft, RaftNode* rNode, char* msg) {
	raft->resetStartTime();

	// prepare
	bool grant = true;
	int currentTerm = raft->getStatus()->getCurrentTerm();
	Status* status = raft->getStatus();
	Log* log = raft->getStatus()->getLog();

	append_entries_rpc* arpc = (append_entries_rpc*)malloc(sizeof(append_entries_rpc));
	str2arpc(msg, arpc);

	// check valid appendEntriesRPC or not
	if (currentTerm > arpc->term ||
		!log->match(arpc->prevLogIndex, arpc->prevLogTerm)
	) {
		grant = false;
	}
	if (status->isCandidate() && currentTerm <= arpc->term) {
		// become Follower
		status->becomeFollower();

		// vote for the sender
		raft->setVote(0);
		status->setVotedFor(arpc->leaderId);

		raft->setLeaderTerm(arpc->term);
	}

	// do if grant == true
	if (grant) {
		while(status->getCurrentTerm() < arpc->term) {
			status->incrementCurrentTerm();
		}
		currentTerm = status->getCurrentTerm();

		//if (log->lastLogIndex > arpc->prevLogIndex+1 )) {
			// delete log[index] : index > arpc->prevLogIndex
		//}

		// push entries to log
		if (arpc->entries[0] > 0) {
			char entryStr[ENTRY_STR_LENGTH];
			memcpy(entryStr, arpc->entries, ENTRY_STR_LENGTH);

			// split
			vector<string> vec = split(entryStr, ENTRY_DELIMITER);

			// add to log
			int term = stoi(vec[0]);
			if (term >= 0) {
				log->add(term, vec[1].c_str());
			}
		}

		// update commitIndex
		if (arpc->leaderCommit > status->getCommitIndex()) {
			status->setCommitIndex(std::min(arpc->leaderCommit, log->lastLogIndex()));
		}
	}

	// send response to the Leader
	char str[MESSAGE_SIZE];
	response_append_entries* rae = (response_append_entries*)malloc(sizeof(response_append_entries));
	raeByFields(rae, currentTerm, grant);
	rae2str(rae, str);
	sendMessage(raft, rNode, str, MESSAGE_SIZE);

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
	char str[MESSAGE_SIZE];
	response_request_vote* rrv = (response_request_vote*)malloc(sizeof(response_request_vote));
	rrvByFields(rrv, raft->getStatus()->getCurrentTerm(), grant);
	rrv2str(rrv, str);
	sendMessage(raft, rNode, str, MESSAGE_SIZE);

	free(rrpc);
	free(rrv);
}
static void responseAppendEntriesReceived(Raft* raft, RaftNode* rNode, char* msg) {
	response_append_entries* rae = (response_append_entries*)malloc(sizeof(response_append_entries));
	str2rae(msg, rae);

	int nIndex = rNode->getNextIndex();
	if (rae->success) {
		if (nIndex+1 <= raft->getStatus()->getLog()->size()) {
			nIndex++;
		}
	} else {
		nIndex--;
	}
	rNode->setNextIndex(nIndex);

	free(rae);
}
static void responseRequestVoteReceived(Raft* raft, RaftNode* rNode, char* msg) {
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

		for (RaftNode* r : *raft->getRaftNodes()) {
			r->setNextIndex( raft->getStatus()->getLog()->size() );
			r->setMatchIndex(-1);
			r->setSentIndex(-1);
		}
	}

	free(rrv);
}
static void requestLocationReceived(Raft* raft, ClientNode* cNode, char* msg) {
	response_request_location* rrl = (response_request_location*)malloc(sizeof(response_request_location));
	char response[MESSAGE_SIZE];
	RaftNode* leader = raft->getLeader();
	rrlByFields(
		rrl,
		leader->getHostname().c_str(),
		leader->getListenPort()
	);
	rrl2str(rrl, response);
	sendMessage(raft, cNode, response, MESSAGE_SIZE);

	free(rrl);
}
static void clientCommandReceived(Raft* raft, ClientNode* cNode, char* msg) {
}

static void sendMessage(Raft* raft, RaftNode* rNode, char* msg, int length) {
	connect2raftnode(raft, rNode);
	rNode->send(msg, length);
}
static void sendMessage(Raft* raft, ClientNode* cNode, char* msg, int length) {
	cNode->send(msg, length);
}

static void connect2raftnode(Raft* raft, RaftNode* rNode) {
	// return if I have already connected to rNode
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
	} else {
		rNode->setSendSock(sock);
	}
}

static void startWorkerThread(Raft* raft, RaftNode* rNode, ClientNode* cNode, bool isClient) {
	pthread_t worker;
	worker_args* wargs = (worker_args*)malloc(sizeof(worker_args));
	wargs->raft     = raft;
	wargs->rNode    = rNode;
	wargs->cNode    = cNode;
	wargs->isClient = isClient;
	if (pthread_create( &worker, NULL, work, (void*)wargs ) < 0) {
		perror("pthread_create");
		exit(1);
	}
	//rNode->setWorker(&worker);
	pthread_detach(worker);
}

// worker thread
static void* work(void* args) {
	worker_args* wargs = (worker_args*)args;

	Raft*       raft     = wargs->raft;
	RaftNode*   rNode    = wargs->rNode;
	ClientNode* cNode    = wargs->cNode;
	bool        isClient = wargs->isClient;

	char buf[MESSAGE_SIZE];
	int sock = (isClient) ? cNode->getReceiveSock() : rNode->getReceiveSock();

	while(1) {
		memset(buf, 0, sizeof(buf));
		if (recv(sock, buf, sizeof(buf), MSG_WAITALL) < 0) {
			perror("recv");
			(isClient) ? cNode->setReceiveSock(-1) : rNode->setReceiveSock(-1);
			break;
		}
		if (buf[0] == '\0') {
			continue;
		}
		RPCKind rpcKind = discernRPC(buf);
		//cout << StrRPCKind(rpcKind) << " from " << rNode->getHostname().c_str() << ": [" << buf << "]\n";

		if (rpcKind < 0) {
			cout << "illegal rpc" << endl;
			(isClient) ? cNode->setReceiveSock(-1) : rNode->setReceiveSock(-1);
			break;

		} else if (rpcKind == RPC_KIND_APPEND_ENTRIES) {
			appendEntriesRecieved(raft, rNode, buf);

		} else if (rpcKind == RPC_KIND_REQUEST_VOTE) {
			requestVoteReceived(raft, rNode, buf);

		} else if (rpcKind == RPC_KIND_RESPONSE_APPEND_ENTRIES) {
			responseAppendEntriesReceived(raft, rNode, buf);

		} else if (rpcKind == RPC_KIND_RESPONSE_REQUEST_VOTE) {
			responseRequestVoteReceived(raft, rNode, buf);

		} else if (rpcKind == RPC_KIND_REQUEST_LOCATION) {
			cout << StrRPCKind(rpcKind) << " from " << cNode->getHostname().c_str() << ": [" << buf << "]\n";
			requestLocationReceived(raft, cNode, buf);

		} else if (rpcKind == RPC_KIND_CLIENT_COMMAND) {
			clientCommandReceived(raft, cNode, buf);

		}
	}
}
