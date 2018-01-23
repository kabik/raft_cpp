#include "raft.h"
#include "state.h"

Raft::Raft(char* configFileName) {
    createConfig(configFileName);
}

void Raft::createConfig(char* configFileName) {
    config = new Config();

    ifstream ifs(configFileName);

    // ファイルが無かったら終了
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
            h->nodename = strs[0];
            h->port = stoi(strs[1]);
            nodes.push_back(h);

            cnt++;
        }
    }
}
