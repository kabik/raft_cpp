#include <iostream>

#include "log.h"

using std::cout;
using std::endl;

Log::Log(string storageDirectoryName) : FileHandler(storageDirectoryName + "log") {
	cout << "The Log file is \"" << this->getFileName() << "\"." << endl;
}

int Log::size() {
	return this->_log.size();
}

int Log::lastLogIndex() {
	return this->_log.size() - 1;
}
int Log::lastLogTerm() {
	if (this->lastLogIndex() < 0) {
		return -1;
	} else {
		return this->_log[this->lastLogIndex()]->term;
	}
}

entry Log::get(int index) {
	return *this->_log[index];
}

void Log::add(int term, char command[COMMAND_STR_LENGTH]) {
	entry* e = (entry*)malloc(sizeof(entry));
	e->term = term;
	memcpy(command, e->command, COMMAND_STR_LENGTH);
	char str[COMMAND_STR_LENGTH];
	entry2str(e, str);

	auto out = this->getOFStream(true);
	*out << str << endl;

	this->_log.push_back(e);
}
