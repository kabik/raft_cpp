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
	return this->_log[this->lastLogIndex()]->term;
}

entry Log::get(int index) {
	return *this->_log[index];
}

void Log::add(int term, string command) {
	entry* e = (entry*)malloc(sizeof(entry));
	e->term = term;
	e->command = new string(command);

	auto out = this->getOFStream(true);
	*out << *entry2str(e) << endl;

	this->_log.push_back(e);
}
