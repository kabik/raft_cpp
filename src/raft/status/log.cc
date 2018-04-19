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
	return this->getTerm(this->lastLogIndex());
}

int Log::getTerm(int index) {
	return (index < 0 || index > this->lastLogIndex()) ?
		-1 : this->get(index)->term;
}

bool Log::match(int index, int term) {
	return index < 0                    ||
	       index > this->lastLogIndex() ||
	       term == this->getTerm(index);
}

entry* Log::get(int index) {
	return this->_log[index];
}

void Log::add(int term, const char command[COMMAND_STR_LENGTH]) {
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
