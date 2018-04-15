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

bool Log::match(int index, int term) {
	if (index == -1 && this->lastLogIndex() == -1 ||
		index > this->lastLogIndex()
	) {
		return true;
	}

	entry* e = this->get(index);
	if (!e) {
		return false;
	} else {
		return term == this->get(index)->term;
	}
}

entry* Log::get(int index) {
	return this->_log[index];
}

void Log::add(int term, char command[COMMAND_STR_LENGTH]) {
	// set string
	entry* e = (entry*)malloc(sizeof(entry));
	e->term = term;
	memcpy(e->command, command, COMMAND_STR_LENGTH);
	char str[COMMAND_STR_LENGTH];
	entry2str(e, str);

	// add to log
	_mtx.lock();

	auto out = this->getOFStream(true);
	*out << str << endl;
	this->_log.push_back(e);

	_mtx.unlock();

	// print
	printAll();
}

void Log::printAll() {
	cout << "---log---\n";
	for (entry* e : this->_log) {
		char str[ENTRY_STR_LENGTH];
		entry2str(e, str);
		cout << str << endl;
	}
	cout << "------\n";
}
