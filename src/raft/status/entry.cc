#ifndef ENTRY_CC
#define ENTRY_CC

typedef struct _entry {
	int term;
	string* command;
} entry;

static string* entry2str(entry* e) {
	return new string(std::to_string(e->term) + " " + *e->command);
}

#endif //ENTRY_CC
