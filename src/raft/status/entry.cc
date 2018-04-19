#ifndef ENTRY_CC
#define ENTRY_CC

#include <stdio.h>

#include "../constant.h"

typedef struct _entry {
	int term;
	char command[COMMAND_STR_LENGTH];
} entry;

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
