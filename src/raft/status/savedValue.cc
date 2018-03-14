#include "savedValue.h"

using std::cout;
using std::endl;

SavedValue::SavedValue(string name, string fileName) : FileHandler(fileName) {
	cout << "The " << name << " file is \"" << fileName << "\"." << endl;

	// load the value
	const int n = 16;
	char str[n];
	this->getIFStream()->getline(str, n);

	this->setName(name);
	this->setValue(str);
}

string SavedValue::getName() {
	return *this->_name;
}
void SavedValue::setName(string name) {
	this->_name = new string(name);
}

string SavedValue::getValue() {
	return *this->_value;
}
void SavedValue::setValue(string value) {
	this->_value = new string(value);
	auto out = getOFStream();
	*out << value << endl;
}
