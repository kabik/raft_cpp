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

ofstream* FileHandler::getOFStream() {
	this->_ofs = new ofstream(this->getFileName().c_str());
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
