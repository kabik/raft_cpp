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
using std::vector;

int main(int argc, char* argv[]) {
	if (argc < 3) {
		cout << "Usage: createInput OUTPUT_FILENAME NUM_OF_LINES UPDATE_RATIO" << endl;
		return 0;
	}

	char* filename = argv[1];
	int num = std::stoi(argv[2]);
	float updateRatio = std::stof(argv[3]);

	cout << "Output file is \"" << filename << "\"." << endl;
	cout << "The number of lines is " << num << "." << endl;
	cout << "The ratio of updates to whole commands is " << updateRatio * 100 << "%" << endl;

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
	vector<char*> keyList;// = new vector<char[KEY_LENGTH]>;
	for (int i = 0; i < num; i++) {
		char commandKind = (keyList.empty() || float(rnd()) / RAND_MAX / 2 < updateRatio) ? UPDATE : READ;
		char key[KEY_LENGTH];

		if (commandKind == UPDATE) {
			for (int j = 0; j < KEY_LENGTH-1; j++) {
				key[j] = 'a' + rnd() % 26;
			}
			key[KEY_LENGTH-1] = '\0';

			char *str = (char*)malloc(KEY_LENGTH);
			strcpy(str, key);
			keyList.push_back(str);
		} else {
			int index = rnd() % keyList.size();
			strcpy(key, keyList[index]);
		}

		ofs << commandKind << COMMAND_DELIMITER << key << COMMAND_DELIMITER << i << endl;
	}
	cout << "done.\n";

	ofs.close();

	return 0;
}
