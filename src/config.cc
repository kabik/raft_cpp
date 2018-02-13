#include <iostream>
#include <fstream>
#include <sstream>

#include "config.h"
#include "functions.cc"

using namespace std;

Config::Config(char* configFileName) {
	this->configFileName = configFileName;

	cout << "Config file is \"" << configFileName << "\"." << endl;

	ifstream ifs(configFileName);

	if (!ifs) {
		cerr << "File \"" << configFileName << "\" cannot be opened." << endl;
		exit(1);
	}

	int cnt = 0;
	string str;
	while (getline(ifs, str)) {
		if (!str.empty()) {
			vector<string> strs = split(str, ':');

			node_conf* h = (node_conf*)malloc(sizeof(node_conf));
			h->hostname = new string(strs[0]);
			h->port = stoi(strs[1]);
			this->nodes.push_back(h);

			cnt++;
		}
	}
	ifs.close();

	this->setNumberOfNodes(cnt);

	cout << "Config file is loaded." << endl;
	cout << "Number of nodes is " << this->getNumberOfNodes() << "." << endl;
	for (node_conf* n : this->nodes) {
		cout << "   " << *n->hostname << ":" << n->port << endl;
	}
}

int Config::getNumberOfNodes() {
	return this->nNodes;
}
void Config::setNumberOfNodes(int nNodes) {
	this->nNodes = nNodes;
}

vector<node_conf*> Config::getNodes() {
	return this->nodes;
}
void Config::setNodes(vector<node_conf*> nodes) {
	this->nodes = nodes;
}
