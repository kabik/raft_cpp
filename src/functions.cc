#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <vector>
#include <string>
#include <sstream>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <random>

#define STR(var) #var

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::stringstream;

// devide a string by a certain letter
vector<string> split(const string &s, char delim) {
	vector<string> elems;
	stringstream ss(s);
	string item;
	while (getline(ss, item, delim)) {
		if (!item.empty()) {
			elems.push_back(item);
		}
	}
	return elems;
}

// mkdir
static int stat_mkdir(const char *filepath, mode_t mode) {
	struct stat sb = {0};
	int rc = 0;

	rc = stat(filepath, &sb);
	if (rc == 0) {
		if (!S_ISDIR(sb.st_mode)) {
			cerr << "Error: Not a directory: " << *filepath << endl;
			return -1;
		}
		return 0;
	}

	rc = mkdir(filepath, mode);
	if (rc < 0) {
		cerr << "Error: mkdir(: " << errno << ") " << strerror(errno) << ": " << *filepath << endl;
		return -1;
	}

	return 0;
}

static int mkdir_path(const char *filepath, mode_t mode) {
	char *p = NULL;
	char *buf = NULL;
	int rc = 0;

	buf = (char *)malloc(strlen(filepath) + 4);
	if (buf == NULL) {
		cerr << "Error: malloc(" << errno << ") " << strerror(errno) << endl;
		return -1;
	}
	strcpy(buf, filepath);

	for (p = strchr(buf+1, '/'); p; p = strchr(p+1, '/')) {
		*p = '\0';
		rc = stat_mkdir(buf, mode);
		if (rc != 0) {
			free(buf);
			return -1;
		}
		*p = '/';
	}

	free(buf);
	return 0;
}

static int mymkdir(const char *filepath) {
	int rc = mkdir_path(filepath, 0755);
	return rc;
}

static int myrand(int min, int max) {
	std::random_device rnd;
	return rnd() % (max - min) + min;
}

#endif //FUNCTIONS_H
