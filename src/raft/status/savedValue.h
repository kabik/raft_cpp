#ifndef SAVEDVALUE_H
#define SAVEDVALUE_H

#include "../../fileHandler.h"

class SavedValue : public FileHandler {
private:
	string* _name;
	string* _value;

public:
	SavedValue(string name, string fileName);

	string getName();
	void setName(string name);

	string getValue();
	void setValue(string value);
};

#include "savedValue.cc"
#endif //SAVEDVALUE_H
