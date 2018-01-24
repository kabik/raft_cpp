#include <stdio.h>
#include <iostream>

#include "raft.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Please specify a config file name." << endl;
        return 0;
    }

    Raft *raft = new Raft();
    raft->createConfig(argv[1]);

    return 0;
}
