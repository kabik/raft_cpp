#include <stdio.h>
#include <iostream>
#include <memory>
#include <thread>
#include <signal.h>

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

	// not stop when this process gets SIGPIPE
	signal(SIGPIPE, SIG_IGN);

	if (auto raft = std::make_shared<Raft>(argv[1])) {
		auto receiveThread = thread([&raft]{ raft->receive(); });
		sleep_for(milliseconds(1000));

		auto timerThread = thread([&raft]{ raft->timer(); });

		// direct input
		//auto inputThread = thread([&raft]{ raft->cli(); });

		receiveThread.join();
		timerThread.join();
		//inputThread.join();
	}

	return 0;
}
