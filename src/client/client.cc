#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <vector>
#include <signal.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <chrono>

#include "../raft/constant.h"
#include "../functions.cc"
#include "../raft/rpc.cc"

using std::cout;
using std::cerr;
using std::endl;
using std::ifstream;
using std::vector;
using std::stoi;

int conn(char* hostname, int port) {
	char port_str[PORT_LENGTH];
	sprintf(port_str, "%d", port);

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

	int err = S_GETADDRINFO(hostname, port_str, &hints, &ai);
	if (err) {
		std::cerr << gai_strerror(err) << std::endl;
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
	}

	S_FREEADDRINFO(ai);

	return fd;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		cout << "Please specify an input file name and a config filename." << endl;
		return 0;
	}

	const int OUTPUT_EACH = 1000;

	// not stop when this process gets SIGPIPE
	signal(SIGPIPE, SIG_IGN);

	char* inputFilename = argv[1];
	char* configFilename = argv[2];

	cout << "Input file is \"" << inputFilename << "\"." << endl;
	cout << "Config file is \"" << configFilename << "\"." << endl;

	// input file
	ifstream ifs(inputFilename);
	if (!ifs) {
		cerr << "File \"" << inputFilename << "\" cannot be opened." << endl;
		exit(1);
	}

	char buf[COMMAND_STR_LENGTH];
	vector<string> commandList;
	while (ifs.getline(buf, COMMAND_STR_LENGTH)) {
		vector<string> vec = split(buf, COMMAND_DELIMITER);
		commandList.push_back(buf);
	}

	// config file
	ifstream ifs2(configFilename);
	if (!ifs2) {
		cerr << "File \"" << configFilename << "\" cannot be opened." << endl;
		exit(1);
	}

	char buf2[HOSTNAME_LENGTH];
	char hostname_port[HOSTNAME_LENGTH]; // hostname:port
	while (ifs2.getline(buf2, HOSTNAME_LENGTH)) {
		if (strlen(buf2) > 0) {
			strcpy(hostname_port, buf2);
		}
	}
	vector<string> v = split(hostname_port, ':');
	char hostname[HOSTNAME_LENGTH]; // hostname
	strcpy(hostname, v[0].c_str());
	int port = stoi(v[1].c_str());

	// connect to a raft server
	cout << "connect to " << hostname << ":" << port << endl;

	int fd = conn(hostname, port);

	// request leader location
	request_location* rl = (request_location*)malloc(sizeof(request_location));
	char smsg[MESSAGE_SIZE];
	rlByFields(rl);
	rl2str(rl, smsg);
	if (S_SEND(fd, smsg, MESSAGE_SIZE, 0) < 0) {
		perror("write");
	}
	free(rl);

	// receive leader location
	char rmsg[MESSAGE_SIZE];
	if (S_RECV(fd, rmsg, MESSAGE_SIZE, 0) < 0) {
		perror("recv");
	}
	cout << rmsg << endl;

	// connect to leader
	response_request_location* rrl = (response_request_location*)malloc(sizeof(response_request_location));
	str2rrl(rmsg, rrl);
	cout << rrl->rpcKind << endl
		<< rrl->hostname << endl
		<< rrl->port << endl;
	if (strcmp(hostname, rrl->hostname) != 0) {
		close(fd);
		cout << "connect to leader " << rrl->hostname << ":" << rrl->port << endl;

		fd = conn(rrl->hostname, rrl->port);
	} else {
		cout << "I have connected to leader.\n";
	}
	free(rrl);

	// send commands and receive commit messages
	client_command* cc = (client_command*)malloc(sizeof(client_command));
	cc->rpcKind = RPC_KIND_CLIENT_COMMAND;

	commit_message* cm = (commit_message*)malloc(sizeof(commit_message));

	// start measurement
	std::chrono::system_clock::time_point start, end;
	start = std::chrono::system_clock::now();

	int cnt = 0;
	unsigned int cid = 0;
	for (int i = 0; i < commandList.size(); i++) {
		strcpy(smsg, "");
		strcpy(cc->command, commandList[i].c_str());
		cc->commandId = cid++;
		cc2str(cc, smsg);

		if (S_SEND(fd, smsg, MESSAGE_SIZE, 0) < 0) {
			perror("write");
			exit(1);
		}
		if (S_RECV(fd, rmsg, sizeof(rmsg), MSG_WAITALL) < 0) {
			perror("recv");
			exit(1);
		}
		str2cm(rmsg, cm);
		cnt = (cnt + 1) % OUTPUT_EACH;
		if (cnt == 0) {
			cout << "commit " << cid << endl;
		}
	}
	end = std::chrono::system_clock::now();
	double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
	cout << "time: " << elapsed / 1000 << " seconds" << endl;
	// finish measurement

	free(cc);
	free(cm);

	// close connection
	S_CLOSE(fd);

	// close ifs
	ifs.close();
	ifs2.close();

	return 0;
}
