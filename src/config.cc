#include <iostream>
#include <fstream>
#include <sstream>

#include "config.h"
#include "functions.cc"

using namespace std;

Config::Config(char* configFileName) {
    this->configFileName = configFileName;

    ifstream ifs(configFileName);

    if (!ifs) {
        cerr << "File \"" << configFileName << "\" cannot be opened." << endl;
        exit(1);
    }

    vector<node_t*> nodes;

    int cnt = 0;
    string str;
    while (getline(ifs, str)) {
        if (!str.empty()) {
            vector<string> strs = split(str, ':');

            node_t* h = (node_t*)calloc(1, sizeof(node_t));
            h->hostname = strs[0];
            h->port = stoi(strs[1]);
            nodes.push_back(h);

            cnt++;
        }
    }
}

int Config::getNumberOfNodes() {
    return nNodes;
}
void Config::setNumberOfNodes(int nNodes) {
    this->nNodes = nNodes;
}

vector<node_t*> Config::getNodes() {
    return nodes;
}
void Config::setNodes(vector<node_t*> nodes) {
    this->nodes = nodes;
}
