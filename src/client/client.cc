#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "../raft/constant.h"
#include "../functions.cc"

using std::cout;
using std::cerr;
using std::endl;
using std::ifstream;
using std::vector;

int main(int argc, char* argv[]) {
	if (argc < 2) {
		cout << "Please specify an input file name." << endl;
		return 0;
	}

	char* filename = argv[1];

	// file read
	cout << "Input file is \"" << *filename << "\"." << endl;

	// open an input file stream
	ifstream ifs(filename);
	if (!ifs) {
		cerr << "File \"" << filename << "\" cannot be opened." << endl;
		exit(1);
	}

	// read file
	char buf[COMMAND_STR_LENGTH];
	vector<string> commandList;
	while (ifs.getline(buf, COMMAND_STR_LENGTH)) {
		vector<string> vec = split(buf, COMMAND_DELIMITER);
		commandList.push_back(buf);
	}

	// connect to a raft server
	

	// send commands
	for (string s: commandList) {
		cout << s << endl;
	}

	ifs.close();

	return 0;
}
