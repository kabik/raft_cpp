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

    vector<host_t*> hosts;

    int cnt = 0;
    string str;
    while (getline(ifs, str)) {
        if (!str.empty()) {
            vector<string> strs = split(str, ':');

            host_t* h = (host_t*)calloc(1, sizeof(host_t));
            h->hostname = strs[0];
            h->port = stoi(strs[1]);
            hosts.push_back(h);

            cnt++;
        }
    }
}
