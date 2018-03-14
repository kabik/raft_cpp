#include <sys/stat.h>
#include <sys/types.h>

#include "status.h"
#include "log.h"
#include "savedValue.h"
#include "../../functions.cc"

using std::cout;
using std::endl;

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
