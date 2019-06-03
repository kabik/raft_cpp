#ifndef ENTRY_CC
#define ENTRY_CC

#include <stdio.h>

#include "../constant.h"
#include "../../functions.cc"

typedef struct _entry {
	int term;
	int conn_id;
	char command[COMMAND_STR_LENGTH];
} entry;

static void fields2entry(entry* e, int term, int conn_id, const char command[COMMAND_STR_LENGTH]) {
	e->term = term;
	e->conn_id = conn_id;
	memcpy(e->command, command, COMMAND_STR_LENGTH);
}

static void str2entry(entry* e, char str[ENTRY_STR_LENGTH]) {
	vector<string> vec = split(str, ENTRY_DELIMITER);
	e->term = stoi(vec[0]);
	e->conn_id = stoi(vec[1]);
	memcpy(e->command, vec[2].c_str(), COMMAND_STR_LENGTH);
}

static void entry2str(entry* e, char str[ENTRY_STR_LENGTH]) {
	sprintf(
		str,
		"%d%c%d%c%s",
		e->term,
		ENTRY_DELIMITER,
		e->conn_id,
		ENTRY_DELIMITER,
		e->command
	);
}

#endif //ENTRY_CC
