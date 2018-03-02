#include <stdio.h>
#include <iostream>
#include <thread>

#include "raft/raft.h"

using std::cout;
using std::endl;
using std::thread;
using std::this_thread::sleep_for;
using std::chrono::milliseconds;

int main(int argc, char* argv[]) {
	if (argc < 2) {
		cout << "Please specify a config file name." << endl;
		return 0;
	}

	Raft *raft = new Raft();
	raft->createConfig(argv[1]);
	raft->setRaftNodesByConfig();

	auto listenThread = thread([&raft]{ raft->listenTCP(); });
	sleep_for(chrono::milliseconds(1000));
	raft->connectOtherRaftNodes();

	auto receiveThread = thread([&raft]{ raft->receive(); });

	receiveThread.join();
	listenThread.join();

	return 0;
}
