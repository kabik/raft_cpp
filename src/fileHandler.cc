#include <iostream>
#include <fstream>
#include <sstream>

#include <stdio.h>
#include <string>

#include "fileHandler.h"

FileHandler::FileHandler(string fileName) {
	this->_fileName = new string(fileName);
}

ifstream* FileHandler::getIFStream() {
	this->_ifs = new ifstream(this->getFileName().c_str());
	return this->_ifs;
}

ofstream* FileHandler::getOFStream(bool append_mode) {
	cout << "_ofs : " << this->_ofs << endl;
	if (this->_ofs || this->_append_mode != append_mode) {
		if (append_mode) {
			this->_ofs = new ofstream(this->getFileName().c_str(), std::ios::app);
		} else {
			this->_ofs = new ofstream(this->getFileName().c_str());
		}
	}
	this->_append_mode = append_mode;
	return this->_ofs;
}

void FileHandler::closeIFStream() {
	this->_ifs->close();
}

void FileHandler::closeOFStream() {
	this->_ofs->close();
}

string FileHandler::getFileName() {
	return *this->_fileName;
}
