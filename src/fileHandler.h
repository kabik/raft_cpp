#ifndef FILEHANDLER_H
#define FILEHANDLER_H

using std::ifstream;
using std::ofstream;

class FileHandler {
private:
	string* _fileName;
	ifstream* _ifs;
	ofstream* _ofs;

public:
	FileHandler(string fileName);

	ifstream* getIFStream();
	ofstream* getOFStream();
	void closeIFStream();
	void closeOFStream();
	string getFileName();
};

#include "fileHandler.cc"
#endif //FILEHANDLER_H
