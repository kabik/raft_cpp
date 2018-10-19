#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>

#include "../raft/constant.h"
#include "../functions.cc"

using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using std::ofstream;
using std::stoi;

int main(int argc, char* argv[]) {
	if (argc < 3) {
		cout << "Please specify the output file name and the number of lines." << endl;
		return 0;
	}

	char* filename = argv[1];
	int num = stoi(argv[2]);

	cout << "Output file is \"" << filename << "\"." << endl;
	cout << "The number of lines is \"" << num << "\"." << endl;

	// create the directory
	struct stat st;
	if (stat(filename, &st) != 0) {
		cout << "Creating the Directory \"" << filename << "\" ..." << flush;
		int rc = mymkdir(filename);
		if (rc == 0) {
			cout << "Success!" << endl;
		} else {
			cout << "Failed." << endl;
		}
	} else {
		cout << "The Directory \"" << filename << "\" already exists." << endl;
	}

	// open an out file stream
	ofstream ofs(filename);
	if (!ofs) {
		cerr << "File \"" << filename << "\" cannot be opened." << endl;
		exit(1);
	}

	// create input file
	cout << "creating..." << flush;
	std::random_device rnd;
	for (int i = 0; i < num; i++) {
		char key[KEY_LENGTH];
		for (int j = 0; j < KEY_LENGTH-1; j++) {
			key[j] = 'a' + rnd() % 26;
		}
		key[KEY_LENGTH-1] = '\0';
		ofs << "put" << COMMAND_DELIMITER << key << COMMAND_DELIMITER << i << endl;
	}
	cout << "done.\n";

	ofs.close();

	return 0;
}
