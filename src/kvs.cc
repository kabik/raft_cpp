KVS::KVS() {

}

int KVS::size() {
	return this->mp.size();
}

void KVS::get(char key[KEY_LENGTH], char value[VALUE_LENGTH]) {
	memcpy(value, this->mp[key], VALUE_LENGTH);
}

void KVS::put(char key[KEY_LENGTH], char value[VALUE_LENGTH]) {
	this->mp[key] = value;
}

void KVS::del(char key[KEY_LENGTH]) {
	this->mp.erase(key);
}

void KVS::printAll() {
	cout << "---kvs---\n";
	for (auto x : this->mp) {
		cout << x.first << " => " << x.second << endl;
	}
	cout << "------\n";
}
