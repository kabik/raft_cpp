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

	this->lastSyncedIndex = this->lastLogIndex();
}

int Log::size() {
	int ret;

	_mtx.lock();
	ret = this->_log.size();
	_mtx.unlock();

	return ret;
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

int Log::getConnectionId(int index) {
	return (index < 0 || index > this->lastLogIndex()) ?
		-1 : this->get(index)->conn_id;
}

bool Log::match(int index, int term) {
	return index < 0                    ||
	       term == this->getTerm(index);
}

entry* Log::get(int index) {
	_mtx.lock();
	entry *e = this->_log[index];
	_mtx.unlock();

	return e;
}

// Leader
void Log::add(int term, int conn_id, const char command[COMMAND_STR_LENGTH]) {
	// set string
	entry* e = NULL;
	while (e == NULL) {
		e = (entry*)malloc(sizeof(entry));
	}
	fields2entry(e, term, conn_id, command);
	char str[COMMAND_STR_LENGTH];
	entry2str(e, str);

	// add to log
	_mtx.lock();

	this->_log.push_back(e);

	_mtx.unlock();
}

// Follower
void Log::add(entry* entries[], int num) {
	_mtx.lock();
	auto out = this->getOFStream(true);
	for (int i = 0; i < num; i++) {
		entry* e = entries[i];
		char str[COMMAND_STR_LENGTH];
		entry2str(e, str);

		// add to log
		*out << str << endl;
		this->_log.push_back(e);
	}
	*out << std::flush;

	_mtx.unlock();
}

int Log::getLastSyncedIndex() {
	return this->lastSyncedIndex;
}

void Log::sync() {
	auto out = this->getOFStream(true);

	int lastIndex = this->lastLogIndex();
	for (int i = this->getLastSyncedIndex() + 1; i <= lastIndex; i++) {
		entry* e = this->get(i);
		char str[COMMAND_STR_LENGTH];
		entry2str(e, str);

		*out << str << endl;
	}
	*out << std::flush;

	this->lastSyncedIndex = lastIndex;

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
