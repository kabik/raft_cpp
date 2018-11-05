KVS::KVS() {

}

int KVS::size() {
	return this->mp.size();
}

void KVS::get(const char key[KEY_LENGTH], char value[VALUE_LENGTH]) {
	memcpy(value, this->mp[key].c_str(), VALUE_LENGTH);
}

void KVS::put(const char key[KEY_LENGTH], const char value[VALUE_LENGTH]) {
	this->mp[string(key)] = string(value);
}

void KVS::del(const char key[KEY_LENGTH]) {
	this->mp.erase(string(key));
}

void KVS::printAll() {
	cout << "---kvs---\n";
	for (auto x : this->mp) {
		cout << x.first << " => " << x.second << endl;
	}
	cout << "---kvs end---\n";
}
