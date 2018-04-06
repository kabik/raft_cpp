#ifndef FILEHANDLER_H
#define FILEHANDLER_H

using std::ifstream;
using std::ofstream;

class FileHandler {
private:
	string* _fileName;
	ifstream* _ifs;
	ofstream* _ofs;
	bool _append_mode;

public:
	FileHandler(string fileName);

	ifstream* getIFStream();
	ofstream* getOFStream(bool append_mode);
	void closeIFStream();
	void closeOFStream();
	string getFileName();
};

#include "fileHandler.cc"
#endif //FILEHANDLER_H
