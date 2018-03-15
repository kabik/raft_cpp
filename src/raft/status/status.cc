#include <sys/stat.h>
#include <sys/types.h>

#include "status.h"
#include "log.h"
#include "savedValue.h"
#include "../../functions.cc"

using std::cout;
using std::endl;
using std::stoi;
using std::to_string;

Status::Status(string storageDirectoryName) {
	this->storageDirectoryName = new string(storageDirectoryName);
	this->createDirectory();

	this->log = new Log(storageDirectoryName);
	this->currentTerm = new SavedValue("currentTerm", storageDirectoryName + "currentTerm");
	this->votedFor = new SavedValue("votedFor", storageDirectoryName + "votedFor");

	cout << this->votedFor->getName() << ": " << this->votedFor->getValue() << endl;
	cout << this->currentTerm->getName() << ": " << this->currentTerm->getValue() << endl;
}

void Status::createDirectory() {
	// crate the directory if it DOES NOT exist
	struct stat st;
	if (stat(this->getStorageDirectoryName().c_str(), &st) != 0) {
		cout << "Creating the Directory \"" << this->getStorageDirectoryName() << "\"." << endl;
		cout << "***";
		int rc = mymkdir(this->getStorageDirectoryName().c_str());
		if (rc == 0) {
			cout << "Success!" << endl;
		} else {
			cout << "Failed" << endl;
		}
	} else {
		cout << "The Directory \"" << this->getStorageDirectoryName() << "\" already exists." << endl;
	}
}

string Status::getStorageDirectoryName() {
	return *this->storageDirectoryName;
}

int Status::getCurrentTerm() {
	return stoi(this->currentTerm->getValue());
}
void Status::incrementCurrentTerm() {
	int cTerm = stoi(this->currentTerm->getValue());
	this->currentTerm->setValue(to_string(cTerm+1));
}

int Status::getVotedFor() {
	stoi(this->votedFor->getValue());
}
void Status::setVotedFor(int node_id) {
	this->votedFor->setValue(to_string(node_id));
}

int Status::getCommitIndex() {
	return this->commitIndex;
}
void Status::setCommitIndex(int commitIndex) {
	this->commitIndex = commitIndex;
}

int Status::getLastApplied() {
	return this->lastApplied;
}
void Status::setLastApplied(int lastApplied) {
	this->lastApplied = lastApplied;
}
