#include <stdio.h>
#include <string.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <chrono>

#include "raft.h"
#include "../config.h"
#include "../kvs.h"
#include "status/status.h"
#include "status/entry.cc"
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

high_resolution_clock::time_point first_log_time;

Raft::Raft(char* configFileName) {
	this->config = new Config(configFileName);
	this->raftNodes = new vector<RaftNode*>;
	this->clientNodes = new vector<ClientNode*>;
	this->setRaftNodesByConfig();

	this->status = new Status(this->getConfig()->getStorageDirectoryName());
	this->status->setState(FOLLOWER);
	this->resetTimeoutTime();

	this->kvs = new KVS();

	// others
	this->leaderTerm = this->status->getCurrentTerm();
	this->vote = 0;

	this->commitCount = 0;
}


void Raft::lock() { _mtx.lock(); }
void Raft::unlock() { _mtx.unlock(); }

int Raft::incrementCommitCount() {
	int ret;
	_mtx.lock();
	ret = ++this->commitCount;
	_mtx.unlock();

	return ret;
}

// to use receive thread
void Raft::receive() {
	int fd;
	struct S_ADDRINFO hints;
	struct S_ADDRINFO *ai;

	memset(&hints, 0, sizeof(hints));
#ifdef ENABLE_RSOCKET
	hints.ai_flags = RAI_PASSIVE | RAI_FAMILY;
	hints.ai_port_space = RDMA_PS_TCP;
#else
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;
#endif

	char port[PORT_LENGTH] = {0};
	sprintf(port, "%d", this->getRaftNodeById(this->getMe())->getListenPort());
	int err = S_GETADDRINFO(nullptr, port, &hints, &ai);
	if(err) {
		cerr << gai_strerror(err) << endl;
		exit(1);
	}

	if ((fd = S_SOCKET(ai->ai_family, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	// set TCP_NODELAY
	int val = 1;
	if (S_SETSOCKOPT(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1) {
		perror("setsockopt (TPC_NODELAY)");
		S_CLOSE(fd);
		S_FREEADDRINFO(ai);
		exit(1);
	}

	// set internal buffer size
	val = (1 << 21);
	if (S_SETSOCKOPT(fd, SOL_SOCKET, SO_SNDBUF, &val, sizeof(val)) == -1) {
		perror("setsockopt (SO_SNDBUF)");
		S_CLOSE(fd);
		S_FREEADDRINFO(ai);
		exit(1);
	}
	if (S_SETSOCKOPT(fd, SOL_SOCKET, SO_RCVBUF, &val, sizeof(val)) == -1) {
		perror("setsockopt (SO_RCVBUF)");
		S_CLOSE(fd);
		S_FREEADDRINFO(ai);
		exit(1);
	}

#ifdef ENABLE_RSOCKET
	val = 0; // optimization for better bandwidth
	if (S_SETSOCKOPT(fd, SOL_RDMA, RDMA_INLINE, &val, sizeof(val)) == -1) {
		perror("setsockopt (RDMA_INLINE)");
		S_CLOSE(fd);
		S_FREEADDRINFO(ai);
		exit(1);
	}
#endif // ENABLE_RSOCKET

	val = 1;
	if (S_SETSOCKOPT(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) == -1) {
		perror("setsockopt (SO_REUSEADDR)");
		S_CLOSE(fd);
		S_FREEADDRINFO(ai);
		exit(1);
	}

	if (S_BIND(fd, S_SRC_ADDR(ai), S_SRC_ADDRLEN(ai)) == -1) {
		perror("bind");
		S_CLOSE(fd);
		S_FREEADDRINFO(ai);
		exit(1);
	}

	if (S_LISTEN(fd, 1024) == -1) {
		perror("listen");
		S_CLOSE(fd);
		S_FREEADDRINFO(ai);
		exit(1);
	}

	// accept
	struct sockaddr_in c_addr;
	socklen_t len = sizeof(c_addr);
	char buf[MESSAGE_SIZE];
	while (1) {
		int client_fd = S_ACCEPT(fd, (sockaddr*)&c_addr, &len);
		if (client_fd == -1) {
			perror("accept");
			S_CLOSE(fd);
			S_FREEADDRINFO(ai);
			exit(1);
		}

		memset(buf, 0, sizeof(buf));
		if (S_RECV(client_fd, buf, sizeof(buf), MSG_WAITALL | MSG_PEEK) < 0) {
			perror("recv");
		}
		RPCKind rpcKind = discernRPC(buf);
		bool isClient = (rpcKind == RPC_KIND_REQUEST_LOCATION || rpcKind == RPC_KIND_CLIENT_COMMAND);

		if (!isClient) {
			for (RaftNode* rNode : *this->getRaftNodes()) {
				if (!rNode->isMe() && inet_ntoa(c_addr.sin_addr) == rNode->getHostname() && rNode->getReceiveSock() < 0) {
					rNode->setReceiveSock(client_fd);
					startWorkerThread(this, rNode, NULL, false);

					cout << rNode->getHostname() << " connected(Raft Node). (sock=" << client_fd << ")\n";
				}
			}
		} else {
			string hostname(inet_ntoa(c_addr.sin_addr));
			ClientNode *cNode = new ClientNode(&hostname, (int)ntohs(c_addr.sin_port));
			cNode->setReceiveSock(client_fd);
			cNode->setSendSock(client_fd);
			startWorkerThread(this, NULL, cNode, true);

			cNode->setID( this->getClientNodes()->size() );
			this->addClientNode(cNode);

			cout << cNode->getHostname() << " connected(Client Node). (sock=" << client_fd << ")\n";
		}
	}
}

// to use timer thread
void Raft::timer() {
	Status* status = this->getStatus();
	Log* log = this->getStatus()->getLog();
	this->resetStartTime();

	while(1) {
		// Leader
		if (status->isLeader()) {
			if (log->getLastSyncedIndex() < log->lastLogIndex()) {
				log->sync();
			}
			if (this->getDuration().count() > HEARTBEAT_INTERVAL) {
				this->resetStartTime();
				for (RaftNode* rNode : *this->getRaftNodes()) {
					if (!rNode->isMe()) {
						this->sendAppendEntriesRPC(rNode, 0, true, false);
					}
				}
			} else {
				for (RaftNode* rNode : *this->getRaftNodes()) {
					if (!rNode->isMe()                                &&
					    log->lastLogIndex() >= rNode->getNextIndex()  &&
					    rNode->getNextIndex() > rNode->getSentIndex()
					) {
						this->sendAppendEntriesRPC(rNode, 0, false, false);
					}
				}
			}


			// send commit message
			for (ClientNode* cNode : *this->getClientNodes()) {
				if (cNode != NULL                                      &&
					status->getLastApplied() > cNode->getCommitIndex() &&
					cNode->getLastIndex() > cNode->getCommitIndex()
				) {
					int lastIndex = cNode->getLastIndex();

					commit_message* cm = (commit_message*)malloc(sizeof(commit_message));
					cmByFields(cm, lastIndex);
					char smsg[MESSAGE_SIZE];
					cm2str(cm, smsg);
					sendMessage(this, cNode, smsg, MESSAGE_SIZE);
					cNode->setCommitIndex(lastIndex);

					free(cm);

					if (this->incrementCommitCount() == MEASURE_LOG_SIZE-1) {
						double elapsed = duration_cast<milliseconds>(high_resolution_clock::now() - first_log_time).count();
						cout << "input time = " << elapsed / 1000 << " sec.\n";
					}
				}
			}
		}

		// All states
		if (this->isTimeout()) {
			cout << "timeout in term " << status->getCurrentTerm() << endl;
			this->resetStartTime();
			if (status->isFollower() || status->isCandidate()) {
				cout << "candidacy" << endl;
				candidacy();
			}
		}

		// apply to statemachine
		while (status->getLastApplied() < status->getCommitIndex()) {
			int applyIndex = status->getLastApplied() + 1;
			this->apply(applyIndex);
			status->setLastApplied(applyIndex);
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
KVS* Raft::getKVS() {
	return this->kvs;
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
RaftNode* Raft::getLeader() {
	return (*this->raftNodes)[this->status->getVotedFor()];
}

vector<ClientNode*>* Raft::getClientNodes() {
	return this->clientNodes;
}
void Raft::addClientNode(ClientNode* cNode) {
	this->clientNodes->push_back(cNode);
}

void Raft::putReadRPCId(int rpcId, int clientId) {
	this->readRPCIds[rpcId] = clientId;
}
void Raft::delReadRPCId(int rpcId) {
	this->readRPCIds.erase(rpcId);
}
int Raft::getClientIdByRpcId(int rpcId) {
	int ret = -1;
	if (this->readRPCIds.find(rpcId) != this->readRPCIds.end()) {
		ret = this->readRPCIds[rpcId];
	}

	return ret;
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

void Raft::apply(int index) {
	entry* e = this->getStatus()->getLog()->get(index);
	KVS* kvs = this->getKVS();

	vector<string> vec = split(e->command, COMMAND_DELIMITER);
	vec.push_back("");

	char key[KEY_LENGTH] = {'\0'}, val[VALUE_LENGTH] = {'\0'};
	vec[1].copy(key, vec[1].size());
	vec[2].copy(val, vec[2].size());

	char commandKind = vec[0][0];
	if        (commandKind == UPDATE) {
		kvs->put(key, val);

	} else if (commandKind == DELETE) {
		kvs->del(key);

	} else {
		cerr << "Unavailable command: " << commandKind << endl;
	}

	//kvs->printAll();
}

void Raft::sendAppendEntriesRPC(RaftNode* rNode, int rpcId, bool isHeartBeat, bool isRequestRead) {
	Status* status = this->getStatus();
	Log* log = status->getLog();

	// send heartbeat
	char msg[MESSAGE_SIZE];
	append_entries_rpc* arpc = (append_entries_rpc*)malloc(sizeof(append_entries_rpc));

	int nextIndex = rNode->getNextIndex();
	int lastIndex = log->lastLogIndex();
	lastIndex = ( lastIndex - nextIndex + 1 <= MAX_NUM_OF_ENTRIES ) ?
		lastIndex : nextIndex + MAX_NUM_OF_ENTRIES - 1;
	lastIndex = ( lastIndex <= log->getLastSyncedIndex() ) ?
		lastIndex : log->getLastSyncedIndex();

	char entriesStr[ENTRIES_STR_LENGTH] = {};
	int strLength = 0;
	if (!isHeartBeat) {
		for (int i = nextIndex; i <= lastIndex; i++) {
			char entryStr[ENTRY_STR_LENGTH] = {};
			entry* e = status->getLog()->get(i);
			entry2str(e, entryStr);

			memcpy(entriesStr + strlen(entriesStr), entryStr, ENTRY_STR_LENGTH);
			if (i < lastIndex) {
				entriesStr[strlen(entriesStr)] = ENTRIES_DELIMITER;
			}
		}
	}

	arpcByFields(
		arpc,                                    // append_entries_rpc* arpc
		status->getCurrentTerm(),                // int term
		this->getMe(),                           // int leaderId
		nextIndex-1,                             // int prevLogIndex
		status->getLog()->getTerm(nextIndex-1),  // int prevLogTerm
		status->getCommitIndex(),                // int leaderCommit
		rpcId,                                   // int rpcId
		isRequestRead,                           // bool isRequestRead
		entriesStr                               // char entries[ENTRIES_STR_LENGTH]
	);
	arpc2str(arpc, msg);
	sendMessage(this, rNode, msg, MESSAGE_SIZE);
	if (!isHeartBeat) {
		rNode->setSentIndex(lastIndex);
	}

	free(arpc);
}


/* === private functions === */
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
		if ('0' <= arpc->entries[0] && arpc->entries[0] <= '9') {
			char entriesStr[ENTRIES_STR_LENGTH];
			memcpy(entriesStr, arpc->entries, ENTRIES_STR_LENGTH);

			vector<string> entriesVec = split(entriesStr, ENTRIES_DELIMITER);
			int numOfEntries = entriesVec.size();
			entry *entries[numOfEntries];

			for (int i = 0; i < numOfEntries; i++) {
				vector<string> vec = split(entriesVec[i].c_str(), ENTRY_DELIMITER);
				entries[i] = (entry*)malloc(sizeof(entry));
				fields2entry(entries[i], stoi(vec[0]), stoi(vec[1]), vec[2].c_str());
			}
			log->add(entries, numOfEntries);
		}

		// update commitIndex
		if (arpc->leaderCommit > status->getCommitIndex()) {
			status->setCommitIndex(std::min(arpc->leaderCommit, log->lastLogIndex()));
		}
	}

	// send response to the Leader
	char str[MESSAGE_SIZE];
	response_append_entries* rae = (response_append_entries*)malloc(sizeof(response_append_entries));
	raeByFields(rae, currentTerm, arpc->rpcId, arpc->isRequestRead, grant);
	rae2str(rae, str);
	sendMessage(raft, rNode, str, MESSAGE_SIZE);

	free(rae);
	free(arpc);
}
static void requestVoteReceived(Raft* raft, RaftNode* rNode, char* msg) {
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
		raft->resetStartTime();
		raft->setLeaderTerm(rrpc->term);
		raft->getStatus()->setVotedFor(rrpc->candidateId);
		cout << "vote for " << rrpc->candidateId << " leaderTerm=" << raft->getLeaderTerm() << " votedFor=" << raft->getStatus()->getVotedFor() << endl;
	} else {
		cout << "reject " << rrpc->candidateId << " leaderTerm=" << raft->getLeaderTerm() << " votedFor=" << raft->getStatus()->getVotedFor() << endl;
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
	Status* status = raft->getStatus();
	int clusterSize = raft->getRaftNodes()->size();

	response_append_entries* rae = (response_append_entries*)malloc(sizeof(response_append_entries));
	str2rae(msg, rae);

	if (rae->isRequestRead) {
		// read request
		raft->lock();
		int clientId = raft->getClientIdByRpcId(rae->rpcId);
		raft->unlock();
		if (clientId >= 0 && rae->success) {
			ClientNode* cNode = raft->getClientNodes()->at(clientId);
			cNode->grant(rNode->getID());

			raft->lock();
			bool canSendCommit =
				cNode->getReadGrantsNum(clusterSize) > clusterSize / 2 &&
				raft->getClientIdByRpcId(rae->rpcId) >= 0;
			if (canSendCommit) {
				cNode->resetReadGrants(clusterSize);
				raft->delReadRPCId(rae->rpcId);
			}
			raft->unlock();

			if (canSendCommit) {
				int lastCommandId = cNode->getLastCommandId();
				commit_message* cm = (commit_message*)malloc(sizeof(commit_message));
				cmByFields(cm, lastCommandId);
				char smsg[MESSAGE_SIZE];
				cm2str(cm, smsg);
				sendMessage(raft, cNode, smsg, MESSAGE_SIZE);
				cNode->setCommittedCommandId(lastCommandId);
				free(cm);

				if (raft->incrementCommitCount() == MEASURE_LOG_SIZE-1) {
					double elapsed = duration_cast<milliseconds>(high_resolution_clock::now() - first_log_time).count();
					cout << "input time = " << elapsed / 1000 << " sec.\n";
				}
			}
		}
	} else {
		int nIndex = rNode->getNextIndex();
		if (rae->success) {
			for (; nIndex <= rNode->getSentIndex(); nIndex++) {
				status->incrementSavedCount(nIndex);

				if (status->getSavedCount(nIndex) > clusterSize / 2 && nIndex > status->getCommitIndex()) {
					status->setCommitIndex(nIndex);
				}
			}
		} else {
			nIndex--;
			rNode->setSentIndex(nIndex);
		}
		rNode->setNextIndex(nIndex);
	}

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
	client_command* cc = (client_command*)malloc(sizeof(client_command));
	str2cc(msg, cc);

	Status* status = raft->getStatus();
	if (status->isLeader()) {
		if (status->getLog()->size() == 0) {
			first_log_time = high_resolution_clock::now();
		}
		cNode->setLastCommandId(cc->commandId);
		if (cc->command[0] == READ) {
			cNode->resetReadGrants( raft->getRaftNodes()->size() );
			int rpcId = myrand(0, RPC_ID_MAX / CLIENTS_MAX) + cNode->getID();
			raft->lock();
			raft->putReadRPCId(rpcId, cNode->getID());
			raft->unlock();

			for (RaftNode* rNode : *raft->getRaftNodes()) {
				if (!rNode->isMe()) {
					raft->sendAppendEntriesRPC(rNode, rpcId, true, true);
				}
			}
		} else {
			status->getLog()->add(status->getCurrentTerm(), cNode->getReceiveSock(), cc->command);
		}
		cNode->setLastIndex(status->getLog()->lastLogIndex());
	} else {
		cout << "I am NOT LEADER!\n";
	}

	free(cc);
}

static void sendMessage(Raft* raft, RaftNode* rNode, char* msg, int length) {
	connect2raftnode(raft, rNode);
	rNode->_send(msg, length);
}
static void sendMessage(Raft* raft, ClientNode* cNode, char* msg, int length) {
	cNode->_send(msg, length);
}

static void connect2raftnode(Raft* raft, RaftNode* rNode) {
	// return if I have already connected to rNode
	if (rNode->getSendSock() > 0) {
		return;
	}

	int fd;
	struct S_ADDRINFO hints;
	struct S_ADDRINFO *ai;

	memset(&hints, 0, sizeof(hints));
#ifdef ENABLE_RSOCKET
	hints.ai_flags = RAI_FAMILY;
	hints.ai_port_space = RDMA_PS_TCP;
#else
	hints.ai_flags = 0;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;
#endif // RNETLIB_ENABLE_VERBS

	char hostname[HOSTNAME_LENGTH];
	strcpy(hostname, rNode->getHostname().c_str());
	char port[PORT_LENGTH];
	sprintf(port, "%d", rNode->getListenPort());

	int err = S_GETADDRINFO(hostname, port, &hints, &ai);
	if (err) {
		cerr << gai_strerror(err) << endl;
		exit(1);
	}

	if ((fd = S_SOCKET(ai->ai_family, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	// set TCP_NODELAY
	int val = 1;
	if (S_SETSOCKOPT(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1) {
		perror("setsockopt (TCP_NODELAY)");
		S_CLOSE(fd);
		S_FREEADDRINFO(ai);
		exit(1);
	}

	// set internal buffer size
	val = (1 << 21);
	if (S_SETSOCKOPT(fd, SOL_SOCKET, SO_SNDBUF, &val, sizeof(val)) == -1) {
		perror("setsockopt (SO_SNDBUF)");
		S_CLOSE(fd);
		S_FREEADDRINFO(ai);
		exit(1);
	}
	if (S_SETSOCKOPT(fd, SOL_SOCKET, SO_RCVBUF, &val, sizeof(val)) == -1) {
		perror("setsockopt (SO_RCVBUF)");
		S_CLOSE(fd);
		S_FREEADDRINFO(ai);
		exit(1);
	}

#ifdef ENABLE_RSOCKET
	val = 0; // optimization for better bandwidth
	if (S_SETSOCKOPT(fd, SOL_RDMA, RDMA_INLINE, &val, sizeof(val)) == -1) {
		perror("setsockopt (RDMA_INLINE)");
		S_CLOSE(fd);
		S_FREEADDRINFO(ai);
		exit(1);
	}
#endif // ENABLE_RSOCKET

	// connect to the server
	if (S_CONNECT(fd, S_DST_ADDR(ai), S_DST_ADDRLEN(ai)) == -1) {
		perror("connect");
		S_CLOSE(fd);
		S_FREEADDRINFO(ai);
		exit(1);
	} else {
		rNode->setSendSock(fd);
	}

	S_FREEADDRINFO(ai);
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
		if (S_RECV(sock, buf, sizeof(buf), MSG_WAITALL) < 0) {
			perror("recv");
			(isClient) ? cNode->setReceiveSock(-1) : rNode->setReceiveSock(-1);
			break;
		}
		if (buf[0] == '\0') {
			continue;
		}
		RPCKind rpcKind = discernRPC(buf);

		if (rpcKind < 0) {
			if (rNode != NULL) {
				cout << "illegal rpc: \"" << buf << "\" from raftNode[" << rNode->getID() << "]" << endl;
			} else {
				cout << "illegal rpc: \"" << buf << "\" from clientNode[" << cNode->getID() << "]" << endl;
			}
			(isClient) ? cNode->setReceiveSock(-1) : rNode->setReceiveSock(-1);
			continue;

		} else if (rpcKind == RPC_KIND_APPEND_ENTRIES) {
			appendEntriesRecieved(raft, rNode, buf);

		} else if (rpcKind == RPC_KIND_REQUEST_VOTE) {
			requestVoteReceived(raft, rNode, buf);

		} else if (rpcKind == RPC_KIND_RESPONSE_APPEND_ENTRIES) {
			responseAppendEntriesReceived(raft, rNode, buf);

		} else if (rpcKind == RPC_KIND_RESPONSE_REQUEST_VOTE) {
			responseRequestVoteReceived(raft, rNode, buf);

		} else if (rpcKind == RPC_KIND_REQUEST_LOCATION) {
			requestLocationReceived(raft, cNode, buf);

		} else if (rpcKind == RPC_KIND_CLIENT_COMMAND) {
			clientCommandReceived(raft, cNode, buf);

		}
	}
}
