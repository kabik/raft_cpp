#ifndef ENTRY_CC
#define ENTRY_CC

#include <stdio.h>

#include "../constant.h"
#include "../../functions.cc"

typedef struct _entry {
	int term;
	char command[COMMAND_STR_LENGTH];
} entry;

static void fields2entry(entry* e, int term, const char command[COMMAND_STR_LENGTH]) {
	e->term = term;
	memcpy(e->command, command, COMMAND_STR_LENGTH);
}

static void str2entry(entry* e, char str[ENTRY_STR_LENGTH]) {
	vector<string> vec = split(str, ENTRY_DELIMITER);
	e->term = stoi(vec[0]);
	memcpy(e->command, vec[1].c_str(), COMMAND_STR_LENGTH);
}

static void entry2str(entry* e, char str[ENTRY_STR_LENGTH]) {
	sprintf(
		str,
		"%d%c%s",
		e->term,
		ENTRY_DELIMITER,
		e->command
	);
}

#endif //ENTRY_CC
