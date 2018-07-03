#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <vector>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../raft/constant.h"
#include "../functions.cc"
#include "../raft/rpc.cc"

using std::cout;
using std::cerr;
using std::endl;
using std::ifstream;
using std::vector;
using std::stoi;

int main(int argc, char* argv[]) {
	if (argc < 2) {
		cout << "Please specify an input file name and a config filename." << endl;
		return 0;
	}

	const int OUTPUT_EACH = 100;

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

	struct sockaddr_in server;

	int sock = socket(AF_INET, SOCK_STREAM, 0);

	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr(hostname);

	if ((connect(sock, (struct sockaddr *)&server, sizeof(server))) < 0) {
		perror("connect");
		exit(1);
	}

	// request leader location
	request_location* rl = (request_location*)malloc(sizeof(request_location));
	char smsg[MESSAGE_SIZE];
	rlByFields(rl);
	rl2str(rl, smsg);
	if (write(sock, smsg, MESSAGE_SIZE) < 0) {
		perror("write");
	}
	free(rl);

	// receive leader location
	char rmsg[MESSAGE_SIZE];
	if (recv(sock, rmsg, MESSAGE_SIZE, MSG_WAITALL) < 0) {
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
		close(sock);
		cout << "connect to leader " << hostname << ":" << port << endl;

		//struct sockaddr_in server;

		sock = socket(AF_INET, SOCK_STREAM, 0);

		server.sin_family = AF_INET;
		server.sin_port = htons(rrl->port);
		server.sin_addr.s_addr = inet_addr(rrl->hostname);

		if ((connect(sock, (struct sockaddr *)&server, sizeof(server))) < 0) {
			perror("connect");
			exit(1);
		}
	} else {
		cout << "I have connected to leader.\n";
	}
	free(rrl);

	// send commands and receive commit messages
	client_command* cc = (client_command*)malloc(sizeof(client_command));
	cc->rpcKind = RPC_KIND_CLIENT_COMMAND;

	commit_message* cm = (commit_message*)malloc(sizeof(commit_message));

	for (int i = 0; i < commandList.size(); i++) {
		strcpy(smsg, "");
		strcpy(cc->command, commandList[i].c_str());
		cc2str(cc, smsg);

		if (write(sock, smsg, MESSAGE_SIZE) < 0) {
			perror("write");
			exit(1);
		}
		if (recv(sock, rmsg, sizeof(rmsg), MSG_WAITALL) < 0) {
			perror("recv");
			exit(1);
		}

		str2cm(rmsg, cm);
		if (cm->commitIndex % OUTPUT_EACH == 0) {
			cout << "commit " << cm->commitIndex << endl;
		}
	}
	free(cc);
	free(cm);

	ifs.close();
	ifs2.close();

	return 0;
}
