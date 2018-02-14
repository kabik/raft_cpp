#include <stdio.h>
#include <iostream>
#include <memory>
#include "raft/raft.h"

using std::cout;
using std::endl;

int main(int argc, char* argv[]) {
	if (argc < 2) {
		cout << "Please specify a config file name." << endl;
		return 0;
	}

	if (auto raft = std::make_shared<Raft>())
	{
		raft->createConfig(argv[1]);
		raft->setRaftNodesByConfig();
	}
	return 0;
}
