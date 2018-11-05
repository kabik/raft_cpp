#ifndef KVS_H
#define KVS_H

#include <map>

#include "raft/constant.h"

using std::map;

class KVS {
private:
	//map<const char*, const char*> mp;
	map<string, string> mp;

public:
	KVS();

	int size();

	void get(const char key[KEY_LENGTH], char value[VALUE_LENGTH]);
	void put(const char key[KEY_LENGTH], const char value[VALUE_LENGTH]);
	void del(const char key[KEY_LENGTH]);

	void printAll();
};

#include "kvs.cc"
#endif //KVS_H
