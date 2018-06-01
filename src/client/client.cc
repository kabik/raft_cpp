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

	int host_max_length = 100;
	char buf2[host_max_length];
	char host[host_max_length];
	while (ifs2.getline(buf2, host_max_length)) {
		if (strlen(buf2) > 0) {
			strcpy(host, buf2);
		}
	}
	vector<string> v = split(host, ':');
	const char *hostname = v[0].c_str();
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
	char msg[MESSAGE_SIZE];
	rlByFields(rl);
	rl2str(rl, msg);
	if (write(sock, msg, MESSAGE_SIZE) < 0) {
		perror("write");
	}
	free(rl);

	// receive leader location
	if (recv(sock, msg, MESSAGE_SIZE, MSG_WAITALL) < 0) {
		perror("recv");
	}
	cout << msg << endl;

	// send commands
	for (string s: commandList) {
		//cout << s << endl;
	}

	ifs.close();
	ifs2.close();

	return 0;
}
