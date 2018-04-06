#include <stdio.h>
#include <iostream>
#include <memory>
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

	if (auto raft = std::make_shared<Raft>()) {
		raft->createConfig(argv[1]);
		raft->setRaftNodesByConfig();
    
    auto receiveThread = thread([&raft]{ raft->receive(); });
    sleep_for(milliseconds(1000));

    auto timerThread = thread([&raft]{ raft->timer(); });
    
    receiveThread.join();
    timerThread.join();
	}

  return 0;
}
