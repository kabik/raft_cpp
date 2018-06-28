#include <iostream>

#include "log.h"
#include "entry.cc"

using std::cout;
using std::endl;

Log::Log(string storageDirectoryName) : FileHandler(storageDirectoryName + "log") {
	cout << "The Log file is \"" << this->getFileName() << "\"." << endl;

	auto in = this->getIFStream();
	char buf[ENTRY_STR_LENGTH];
	while(in && in->getline(buf, ENTRY_STR_LENGTH)) {
		entry* e = NULL;
		while (e == NULL) {
			e = (entry*)malloc(sizeof(entry));
		}
		str2entry(e, buf);
		this->_log.push_back(e);
	}
	printAll();

	closeIFStream();
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
	       term == this->getTerm(index);
}

entry* Log::get(int index) {
	return this->_log[index];
}

void Log::add(int term, const char command[COMMAND_STR_LENGTH]) {
	// set string
	entry* e = NULL;
	while (e == NULL) {
		e = (entry*)malloc(sizeof(entry));
	}
	fields2entry(e, term, command);
	char str[COMMAND_STR_LENGTH];
	entry2str(e, str);

	// add to log
	_mtx.lock();

	auto out = this->getOFStream(true);
	*out << str << endl;
	this->_log.push_back(e);

	_mtx.unlock();

	// print
	//cout << "log[" << this->lastLogIndex() << "] = \"" << str << "\"" << endl;
	//printAll();
}

void Log::printAll() {
	cout << "---log---\n";
	for (entry* e : this->_log) {
		char str[ENTRY_STR_LENGTH];
		entry2str(e, str);
		cout << str << endl;
	}
	cout << "---log end---\n";
}
