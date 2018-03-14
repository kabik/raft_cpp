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

	Raft *raft = new Raft(argv[1]);

	auto listenThread = thread([&raft]{ raft->listenTCP(); });
	sleep_for(milliseconds(1000));
	raft->connectOtherRaftNodes();

	auto receiveThread = thread([&raft]{ raft->receive(); });

	// test
	while (1) {
		sleep_for(milliseconds(1000));
		for (RaftNode* rNode : raft->getRaftNodes()) {
			write(rNode->getSock(), raft->getRaftNodes()[raft->getMe()]->getHostname().c_str(), 30);
		}
	}

	receiveThread.join();
	listenThread.join();

	return 0;
}
