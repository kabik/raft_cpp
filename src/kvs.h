#ifndef KVS_H
#define KVS_H

#include <map>

#include "raft/constant.h"

using std::map;

class KVS {
private:
	map<char*, char*> mp;

public:
	KVS();

	int size();

	void get(char key[KEY_LENGTH], char value[VALUE_LENGTH]);
	void put(char key[KEY_LENGTH], char value[VALUE_LENGTH]);
	void del(char key[KEY_LENGTH]);

	void printAll();
};

#include "kvs.cc"
#endif //KVS_H
